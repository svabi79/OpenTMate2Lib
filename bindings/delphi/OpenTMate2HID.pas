unit OpenTMate2HID;

{
  OpenTMate2HID — Windows-native USB HID transport for the TMate 2.

  Replaces the open/close/read/write surface of the proprietary TMATE2_DLL.dll
  using only SetupAPI + hid.dll (no third-party hidapi). Pair it with the
  OpenTMate2 unit, which owns all protocol logic: feed ReadInput's report to
  OpenTMate2ParseInputReport, and hand WriteLcd a 44-byte LCDVector you built
  with the OpenTMate2 display helpers.

  Report-ID handling (matches the behaviour proven via hidapi):
    * The device's OUT report is the 64-byte LCDVector report, unnumbered.
      Windows WriteFile expects a leading report-ID byte, so WriteLcd prepends
      a 0 -> OutputReportByteLength is 65 on a typical TMate 2.
    * ReadFile likewise returns a leading report-ID byte; ReadInput skips it so
      the parsed report starts at the device byte observed as 0x01.
  These offsets are the one thing to re-confirm on real hardware; they are
  isolated here, not spread through the protocol code.

  Windows only. Build with a Win32/Win64 Delphi target.
}

interface

uses
  Winapi.Windows, System.SysUtils, OpenTMate2;

type
  TOpenTMate2HID = class
  private
    FHandle: THandle;
    FInputLen: Integer;   { Windows InputReportByteLength (incl. report-ID byte) }
    FOutputLen: Integer;  { Windows OutputReportByteLength (incl. report-ID byte) }
    function MatchesDevice(DeviceHandle: THandle): Boolean;
    function FindDevicePath(out Path: string): Boolean;
  public
    constructor Create;
    destructor Destroy; override;

    { Open the first connected TMate 2. Returns False if none is present. }
    function Open: Boolean;
    procedure Close;
    function IsOpen: Boolean;

    { Read one input report. Returns False on timeout or error. On success,
      Input holds the parsed encoders + key bitmask. }
    function ReadInput(out Input: TOpenTMate2Input; TimeoutMs: Cardinal = 50): Boolean;

    { Send a 44-byte LCDVector to the display. Returns False if the vector is
      the wrong size, the device is closed, or the write fails. }
    function WriteLcd(const LCDVector: TBytes): Boolean;
  end;

implementation

{ ── SetupAPI ──────────────────────────────────────────────────────────── }

const
  setupapi = 'setupapi.dll';
  hidDll   = 'hid.dll';

  DIGCF_PRESENT         = $00000002;
  DIGCF_DEVICEINTERFACE = $00000010;

type
  TSPDeviceInterfaceData = record
    cbSize: DWORD;
    InterfaceClassGuid: TGUID;
    Flags: DWORD;
    Reserved: ULONG_PTR;
  end;
  PSPDeviceInterfaceData = ^TSPDeviceInterfaceData;

  { Fixed-size detail buffer (MAX_PATH wide chars is plenty for a device path). }
  TSPDeviceInterfaceDetailDataW = record
    cbSize: DWORD;
    DevicePath: array[0..259] of WideChar;
  end;

function SetupDiGetClassDevsW(ClassGuid: PGUID; Enumerator: PWideChar;
  hwndParent: HWND; Flags: DWORD): THandle; stdcall; external setupapi;
function SetupDiEnumDeviceInterfaces(DeviceInfoSet: THandle; DeviceInfoData: Pointer;
  InterfaceClassGuid: PGUID; MemberIndex: DWORD;
  var DeviceInterfaceData: TSPDeviceInterfaceData): BOOL; stdcall; external setupapi;
function SetupDiGetDeviceInterfaceDetailW(DeviceInfoSet: THandle;
  DeviceInterfaceData: PSPDeviceInterfaceData; DeviceInterfaceDetailData: Pointer;
  DetailSize: DWORD; RequiredSize: PDWORD; DeviceInfoData: Pointer): BOOL;
  stdcall; external setupapi;
function SetupDiDestroyDeviceInfoList(DeviceInfoSet: THandle): BOOL;
  stdcall; external setupapi;

{ ── HID ───────────────────────────────────────────────────────────────── }

type
  THIDDAttributes = record
    Size: DWORD;
    VendorID: Word;
    ProductID: Word;
    VersionNumber: Word;
  end;

  THIDPCaps = record
    Usage: Word;
    UsagePage: Word;
    InputReportByteLength: Word;
    OutputReportByteLength: Word;
    FeatureReportByteLength: Word;
    Reserved: array[0..16] of Word;
    NumberLinkCollectionNodes: Word;
    NumberInputButtonCaps: Word;
    NumberInputValueCaps: Word;
    NumberInputDataIndices: Word;
    NumberOutputButtonCaps: Word;
    NumberOutputValueCaps: Word;
    NumberOutputDataIndices: Word;
    NumberFeatureButtonCaps: Word;
    NumberFeatureValueCaps: Word;
    NumberFeatureDataIndices: Word;
  end;

const
  HIDP_STATUS_SUCCESS = LongInt($00110000);

procedure HidD_GetHidGuid(var HidGuid: TGUID); stdcall; external hidDll;
function HidD_GetAttributes(HidDeviceObject: THandle;
  var Attributes: THIDDAttributes): BOOL; stdcall; external hidDll;
function HidD_GetPreparsedData(HidDeviceObject: THandle;
  var PreparsedData: Pointer): BOOL; stdcall; external hidDll;
function HidD_FreePreparsedData(PreparsedData: Pointer): BOOL; stdcall; external hidDll;
function HidP_GetCaps(PreparsedData: Pointer; var Capabilities: THIDPCaps): LongInt;
  stdcall; external hidDll;

{ ── Implementation ────────────────────────────────────────────────────── }

constructor TOpenTMate2HID.Create;
begin
  inherited Create;
  FHandle := INVALID_HANDLE_VALUE;
  FInputLen := 0;
  FOutputLen := 0;
end;

destructor TOpenTMate2HID.Destroy;
begin
  Close;
  inherited Destroy;
end;

function TOpenTMate2HID.IsOpen: Boolean;
begin
  Result := FHandle <> INVALID_HANDLE_VALUE;
end;

function TOpenTMate2HID.MatchesDevice(DeviceHandle: THandle): Boolean;
var
  Attr: THIDDAttributes;
begin
  Result := False;
  FillChar(Attr, SizeOf(Attr), 0);
  Attr.Size := SizeOf(Attr);
  if HidD_GetAttributes(DeviceHandle, Attr) then
    Result := (Attr.VendorID = OPENTMATE2_VENDOR_ID) and
              (Attr.ProductID = OPENTMATE2_PRODUCT_ID);
end;

function TOpenTMate2HID.FindDevicePath(out Path: string): Boolean;
var
  HidGuid: TGUID;
  DevInfo: THandle;
  IfData: TSPDeviceInterfaceData;
  Detail: TSPDeviceInterfaceDetailDataW;
  Index: DWORD;
  Probe: THandle;
begin
  Result := False;
  Path := '';

  HidD_GetHidGuid(HidGuid);
  DevInfo := SetupDiGetClassDevsW(@HidGuid, nil, 0,
    DIGCF_PRESENT or DIGCF_DEVICEINTERFACE);
  if DevInfo = THandle(INVALID_HANDLE_VALUE) then
    Exit;
  try
    Index := 0;
    while True do
    begin
      FillChar(IfData, SizeOf(IfData), 0);
      IfData.cbSize := SizeOf(IfData);
      if not SetupDiEnumDeviceInterfaces(DevInfo, nil, @HidGuid, Index, IfData) then
        Break; { no more interfaces }
      Inc(Index);

      FillChar(Detail, SizeOf(Detail), 0);
      {$IFDEF WIN64}
      Detail.cbSize := 8;
      {$ELSE}
      Detail.cbSize := 6;
      {$ENDIF}
      if not SetupDiGetDeviceInterfaceDetailW(DevInfo, @IfData, @Detail,
        SizeOf(Detail), nil, nil) then
        Continue;

      { Open just to read VID/PID, then keep it only if it matches. }
      Probe := CreateFile(PChar(@Detail.DevicePath[0]), 0,
        FILE_SHARE_READ or FILE_SHARE_WRITE, nil, OPEN_EXISTING, 0, 0);
      if Probe = INVALID_HANDLE_VALUE then
        Continue;
      try
        if MatchesDevice(Probe) then
        begin
          Path := PChar(@Detail.DevicePath[0]);
          Result := True;
          Break;
        end;
      finally
        CloseHandle(Probe);
      end;
    end;
  finally
    SetupDiDestroyDeviceInfoList(DevInfo);
  end;
end;

function TOpenTMate2HID.Open: Boolean;
var
  Path: string;
  Preparsed: Pointer;
  Caps: THIDPCaps;
begin
  Result := False;
  if IsOpen then
  begin
    Result := True;
    Exit;
  end;

  if not FindDevicePath(Path) then
    Exit;

  FHandle := CreateFile(PChar(Path), GENERIC_READ or GENERIC_WRITE,
    FILE_SHARE_READ or FILE_SHARE_WRITE, nil, OPEN_EXISTING,
    FILE_FLAG_OVERLAPPED, 0);
  if FHandle = INVALID_HANDLE_VALUE then
    Exit;

  { Discover report lengths so read/write match the device exactly. }
  Preparsed := nil;
  if HidD_GetPreparsedData(FHandle, Preparsed) then
  try
    FillChar(Caps, SizeOf(Caps), 0);
    if HidP_GetCaps(Preparsed, Caps) = HIDP_STATUS_SUCCESS then
    begin
      FInputLen := Caps.InputReportByteLength;
      FOutputLen := Caps.OutputReportByteLength;
    end;
  finally
    HidD_FreePreparsedData(Preparsed);
  end;

  { Fall back to the observed 65-byte framing if caps were unavailable. }
  if FInputLen <= 0 then
    FInputLen := OPENTMATE2_REPORT_SIZE + 1;
  if FOutputLen <= 0 then
    FOutputLen := OPENTMATE2_REPORT_SIZE + 1;

  Result := True;
end;

procedure TOpenTMate2HID.Close;
begin
  if FHandle <> INVALID_HANDLE_VALUE then
  begin
    CloseHandle(FHandle);
    FHandle := INVALID_HANDLE_VALUE;
  end;
  FInputLen := 0;
  FOutputLen := 0;
end;

function TOpenTMate2HID.ReadInput(out Input: TOpenTMate2Input;
  TimeoutMs: Cardinal): Boolean;
var
  Raw: TBytes;
  Payload: TBytes;
  Ov: TOverlapped;
  BytesRead: DWORD;
  WaitRes: DWORD;
  PayloadLen: Integer;
begin
  Result := False;
  if not IsOpen then
    Exit;

  SetLength(Raw, FInputLen);
  FillChar(Ov, SizeOf(Ov), 0);
  Ov.hEvent := CreateEvent(nil, True, False, nil);
  if Ov.hEvent = 0 then
    Exit;
  try
    BytesRead := 0;
    if ReadFile(FHandle, Raw[0], FInputLen, BytesRead, @Ov) then
    begin
      { Completed synchronously. }
    end
    else if GetLastError = ERROR_IO_PENDING then
    begin
      WaitRes := WaitForSingleObject(Ov.hEvent, TimeoutMs);
      if WaitRes <> WAIT_OBJECT_0 then
      begin
        CancelIo(FHandle);
        Exit; { timeout or wait failure }
      end;
      if not GetOverlappedResult(FHandle, Ov, BytesRead, True) then
        Exit;
    end
    else
      Exit; { read failed outright }

    { Drop the leading Windows report-ID byte so the parser sees the device
      report starting at the byte observed as 0x01. }
    if BytesRead < 2 then
      Exit;
    PayloadLen := Integer(BytesRead) - 1;
    SetLength(Payload, PayloadLen);
    Move(Raw[1], Payload[0], PayloadLen);
    Result := OpenTMate2ParseInputReport(Payload, Input);
  finally
    CloseHandle(Ov.hEvent);
  end;
end;

function TOpenTMate2HID.WriteLcd(const LCDVector: TBytes): Boolean;
var
  Report: TBytes;
  WrBuf: TBytes;
  Ov: TOverlapped;
  BytesWritten: DWORD;
  CopyLen: Integer;
begin
  Result := False;
  if not IsOpen then
    Exit;
  if not OpenTMate2BuildOutputReport(LCDVector, Report) then
    Exit; { wrong LCDVector size }

  { Prepend the report-ID byte (0) the Windows HID stack expects. }
  SetLength(WrBuf, FOutputLen);
  FillChar(WrBuf[0], FOutputLen, 0);
  CopyLen := Length(Report);
  if CopyLen > FOutputLen - 1 then
    CopyLen := FOutputLen - 1;
  Move(Report[0], WrBuf[1], CopyLen);

  FillChar(Ov, SizeOf(Ov), 0);
  Ov.hEvent := CreateEvent(nil, True, False, nil);
  if Ov.hEvent = 0 then
    Exit;
  try
    BytesWritten := 0;
    if WriteFile(FHandle, WrBuf[0], FOutputLen, BytesWritten, @Ov) then
      Result := True
    else if GetLastError = ERROR_IO_PENDING then
    begin
      if WaitForSingleObject(Ov.hEvent, 1000) = WAIT_OBJECT_0 then
        Result := GetOverlappedResult(FHandle, Ov, BytesWritten, True)
      else
        CancelIo(FHandle);
    end;
  finally
    CloseHandle(Ov.hEvent);
  end;
end;

end.
