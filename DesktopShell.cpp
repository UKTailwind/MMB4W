// Native desktop menus, portable input profiles, and emulator settings.
// This product includes software developed by Geoff Graham and Peter Mather.
#include "DesktopShell.h"
#include <commdlg.h>
#include <shellapi.h>
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cwctype>
#include <mutex>
#include <vector>

namespace {
enum Command {
    OpenProgram=41001, ResetProgram=41003, ExitApplication,
    PauseEmulation, InputSettings, Screenshot, Fullscreen, MuteAudio,
    CaptureMouse, ControlsHelp, AboutEmulator,
    Scale1=41030, Scale2, Scale3, Scale4,
    Volume0=41040, Volume25, Volume50, Volume75, Volume100,
    ProfileDefault=41050, ProfileGauntlet, ProfileCustom,
    RecentFirst=41100
};
enum Control {
    ProfileChoice=101, PageKeyboard=110, PageMouse, PageGamepad, PageGeneral,
    MappingList=120, GuestKeyChoice, CaptureKey, RemoveMapping,
    MouseEnable=140, MouseLeft, MouseRight, MouseMiddle, WheelUp, WheelDown,
    MouseMotion, MouseCapture, MouseDeadZone,
    GamepadEnable=160, GamepadDeadZone, GamepadFirst=170,
    ScaleChoice=190, FullscreenChoice, VolumeEdit, MutedChoice,
    RestoreDefaults=200, SaveCustom, ApplySettings, AcceptSettings, CancelSettings
};
std::once_flag gInitialize;
std::mutex gSettingsMutex, gFileMutex, gProgramMutex;
DesktopSettings gSettings;
std::atomic<HWND> gWindow{nullptr};
std::atomic<bool> gPaused{false}, gQuit{false}, gScreenshot{false}, gDisplayChange{false};
std::atomic<bool> gCaptureReleased{false};
std::atomic<bool> gInMenu{false};
std::atomic<float> gVolume{0.8f};
std::atomic<uint64_t> gRevision{1};
std::wstring gDirectory, gExecutable, gIni, gProgram;
std::vector<std::wstring> gRecent;
bool gNoAudio=false;
HWND gDialog=nullptr;
bool gHotkeyDown[256]{};

int Clamp(int value, int low, int high) { return std::max(low,std::min(high,value)); }
std::wstring Number(int value) { return std::to_wstring(value); }
bool IsFile(const std::wstring& name) {
    DWORD attributes=GetFileAttributesW(name.c_str());
    return attributes!=INVALID_FILE_ATTRIBUTES && !(attributes&FILE_ATTRIBUTE_DIRECTORY);
}
std::wstring Parent(const std::wstring& name) {
    size_t slash=name.find_last_of(L"\\/");
    return slash==std::wstring::npos ? L"." : name.substr(0,slash);
}
std::wstring IniString(const std::wstring& section,const std::wstring& key,const std::wstring& fallback=L"") {
    wchar_t value[2048]{};
    GetPrivateProfileStringW(section.c_str(),key.c_str(),fallback.c_str(),value,2048,gIni.c_str());
    return value;
}
int IniInt(const std::wstring& section,const std::wstring& key,int fallback,int low,int high) {
    std::wstring value=IniString(section,key,Number(fallback));
    wchar_t* end=nullptr;
    long number=wcstol(value.c_str(),&end,10);
    if(end==value.c_str() || *end || number<low || number>high) return fallback;
    return static_cast<int>(number);
}
void WriteIni(const std::wstring& section,const std::wstring& key,const std::wstring& value) {
    WritePrivateProfileStringW(section.c_str(),key.c_str(),value.c_str(),gIni.c_str());
}
bool ValidProfile(const std::wstring& name) {
    return name==L"Default" || name==L"Gauntlet" || name==L"Custom";
}
DesktopSettings Defaults(const std::wstring& profile) {
    DesktopSettings settings;
    settings.profile=ValidProfile(profile)?profile:L"Default";
    for(int key=0;key<256;++key) settings.keyboard[key]=key;
    if(settings.profile==L"Gauntlet") settings.mouseButtonMap={{32,77,0}};
    return settings;
}
DesktopSettings ReadProfile(const std::wstring& profile) {
    DesktopSettings settings=Defaults(profile);
    const std::wstring section=L"Profile."+settings.profile;
    for(int key=1;key<256;++key)
        settings.keyboard[key]=IniInt(section,L"Key"+Number(key),key,0,255);
    const wchar_t* buttons[]={L"MouseLeft",L"MouseRight",L"MouseMiddle"};
    for(int i=0;i<3;++i)settings.mouseButtonMap[i]=IniInt(section,buttons[i],settings.mouseButtonMap[i],0,255);
    settings.mouseWheelUp=IniInt(section,L"WheelUp",0,0,255);
    settings.mouseWheelDown=IniInt(section,L"WheelDown",0,0,255);
    settings.mouseEnabled=IniInt(section,L"MouseEnabled",1,0,1)!=0;
    settings.mouseMotion=IniInt(section,L"MouseMotion",0,0,1)!=0;
    settings.captureMouse=IniInt(section,L"MouseCapture",0,0,1)!=0;
    settings.mouseDeadZone=IniInt(section,L"MouseDeadZone",24,1,200);
    settings.gamepadEnabled=IniInt(section,L"GamepadEnabled",1,0,1)!=0;
    settings.gamepadDeadZone=IniInt(section,L"GamepadDeadZone",10000,0,30000);
    for(int i=0;i<DesktopGamepadActions;++i)
        settings.gamepadMap[i]=IniInt(section,L"Gamepad"+Number(i),settings.gamepadMap[i],0,255);
    return settings;
}
void SaveSettings(const DesktopSettings& settings) {
    std::lock_guard<std::mutex> fileLock(gFileMutex);
    WriteIni(L"Emulator",L"Profile",settings.profile);
    WriteIni(L"Display",L"Scale",Number(settings.scale));
    WriteIni(L"Display",L"Fullscreen",Number(settings.fullscreen));
    WriteIni(L"Audio",L"Volume",Number(settings.volume));
    WriteIni(L"Audio",L"Muted",Number(settings.muted));
    const std::wstring section=L"Profile."+settings.profile;
    for(int key=1;key<256;++key)WriteIni(section,L"Key"+Number(key),Number(settings.keyboard[key]));
    const wchar_t* buttons[]={L"MouseLeft",L"MouseRight",L"MouseMiddle"};
    for(int i=0;i<3;++i)WriteIni(section,buttons[i],Number(settings.mouseButtonMap[i]));
    WriteIni(section,L"WheelUp",Number(settings.mouseWheelUp));
    WriteIni(section,L"WheelDown",Number(settings.mouseWheelDown));
    WriteIni(section,L"MouseEnabled",Number(settings.mouseEnabled));
    WriteIni(section,L"MouseMotion",Number(settings.mouseMotion));
    WriteIni(section,L"MouseCapture",Number(settings.captureMouse));
    WriteIni(section,L"MouseDeadZone",Number(settings.mouseDeadZone));
    WriteIni(section,L"GamepadEnabled",Number(settings.gamepadEnabled));
    WriteIni(section,L"GamepadDeadZone",Number(settings.gamepadDeadZone));
    for(int i=0;i<DesktopGamepadActions;++i)WriteIni(section,L"Gamepad"+Number(i),Number(settings.gamepadMap[i]));
}
void UpdateMenu(HWND window);
void Apply(const DesktopSettings& settings,bool persist=true) {
    bool displayChanged;
    {
        std::lock_guard<std::mutex> lock(gSettingsMutex);
        displayChanged=settings.scale!=gSettings.scale || settings.fullscreen!=gSettings.fullscreen;
        gSettings=settings;
    }
    gVolume.store(settings.muted?0.0f:static_cast<float>(settings.volume)/100.0f);
    gCaptureReleased.store(false);
    ++gRevision;
    if(displayChanged)gDisplayChange.store(true);
    if(persist)SaveSettings(settings);
    HWND window=gWindow.load();
    if(window)PostMessageW(window,WM_APP+414,0,0);
}
void CopyDisplay(const DesktopSettings& from,DesktopSettings& to) {
    to.scale=from.scale;to.fullscreen=from.fullscreen;to.volume=from.volume;to.muted=from.muted;
}
std::wstring KeyName(int key) {
    switch(key) {
    case 0:return L"None / disabled";
    case 8:return L"Backspace";
    case 9:return L"Tab";
    case 13:return L"Enter / confirm";
    case 27:return L"Escape";
    case 32:return L"Space / fire";
    case 77:return L"M / magic";
    case 127:return L"Delete";
    case 128:return L"Up";
    case 129:return L"Down";
    case 130:return L"Left";
    case 131:return L"Right";
    case 132:return L"Insert";
    case 134:return L"Home";
    case 135:return L"End";
    case 136:return L"Page Up";
    case 137:return L"Page Down";
    default:
        if(key>=145 && key<=156)return L"F"+Number(key-144);
        if(key>=33 && key<=126)return std::wstring(1,static_cast<wchar_t>(key));
        return L"BASIC key "+Number(key);
    }
}
int VkToBasic(WPARAM key) {
    if((key>='A'&&key<='Z') || (key>='0'&&key<='9'))return static_cast<int>(key);
    if(key>=VK_NUMPAD0 && key<=VK_NUMPAD9)return '0'+static_cast<int>(key-VK_NUMPAD0);
    if(key>=VK_F1 && key<=VK_F12)return 145+static_cast<int>(key-VK_F1);
    switch(key) {
    case VK_UP:return 128;case VK_DOWN:return 129;case VK_LEFT:return 130;case VK_RIGHT:return 131;
    case VK_SPACE:return 32;case VK_RETURN:return 13;case VK_ESCAPE:return 27;case VK_TAB:return 9;
    case VK_BACK:return 8;case VK_INSERT:return 132;case VK_DELETE:return 127;
    case VK_HOME:return 134;case VK_END:return 135;case VK_PRIOR:return 136;case VK_NEXT:return 137;
    case VK_MULTIPLY:return '*';case VK_DIVIDE:return '/';case VK_ADD:return '+';case VK_SUBTRACT:return '-';case VK_DECIMAL:return '.';
    default:
        UINT character=MapVirtualKeyW(static_cast<UINT>(key),MAPVK_VK_TO_CHAR)&0xffff;
        return character>=33 && character<=126 ? static_cast<int>(towupper(static_cast<wchar_t>(character))) : -1;
    }
}
void AddRecent(const std::wstring& path) {
    if(path.empty())return;
    std::lock_guard<std::mutex> fileLock(gFileMutex);
    gRecent.erase(std::remove_if(gRecent.begin(),gRecent.end(),[&](const std::wstring& item){return _wcsicmp(item.c_str(),path.c_str())==0;}),gRecent.end());
    gRecent.insert(gRecent.begin(),path);
    if(gRecent.size()>5)gRecent.resize(5);
    for(size_t i=0;i<5;++i)WriteIni(L"Recent",Number(static_cast<int>(i)),i<gRecent.size()?gRecent[i]:L"");
    HWND window=gWindow.load();if(window)PostMessageW(window,WM_APP+415,0,0);
}
std::wstring Quote(const std::wstring& value) {
    std::wstring result=L"\"";
    size_t slashes=0;
    for(wchar_t c:value) {
        if(c==L'\\'){++slashes;continue;}
        result.append(slashes*(c==L'"'?2:1),L'\\');
        if(c==L'"')result+=L'\\';
        result+=c;slashes=0;
    }
    result.append(slashes*2,L'\\');result+=L'"';return result;
}
bool LaunchProgram(HWND window,const std::wstring& path) {
    size_t dot=path.find_last_of(L'.');
    if(!IsFile(path) || dot==std::wstring::npos || _wcsicmp(path.substr(dot).c_str(),L".bas")!=0) {
        MessageBoxW(window,L"Select an existing MMBasic .BAS program.",L"Open program",MB_OK|MB_ICONWARNING);return false;
    }
    if(path.size()>235 || path.find(L',')!=std::wstring::npos) {
        MessageBoxW(window,L"This MMBasic runtime needs a program path shorter than 236 characters without commas. Move the program to a shorter folder path and try again.",L"Open program",MB_OK|MB_ICONWARNING);return false;
    }
    std::wstring command=Quote(gExecutable)+(gNoAudio?L" 0 ":L" ")+Quote(path);
    std::vector<wchar_t> writable(command.begin(),command.end());writable.push_back(0);
    STARTUPINFOW startup{};startup.cb=sizeof(startup);
    PROCESS_INFORMATION process{};
    const std::wstring directory=Parent(path);
    if(!CreateProcessW(gExecutable.c_str(),writable.data(),nullptr,nullptr,FALSE,0,nullptr,directory.c_str(),&startup,&process)) {
        MessageBoxW(window,L"Windows could not start the selected program. Check that the emulator and program files are still present.",L"Open program",MB_OK|MB_ICONERROR);return false;
    }
    CloseHandle(process.hThread);CloseHandle(process.hProcess);
    AddRecent(path);gQuit.store(true);return true;
}
void OpenDialog(HWND window) {
    bool previous=gPaused.exchange(true);
    DesktopUpdateMouseCapture(window,false);
    wchar_t name[32768]{};
    OPENFILENAMEW dialog{};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=window;
    dialog.lpstrFilter=L"MMBasic programs (*.BAS)\0*.bas\0All files\0*.*\0";
    dialog.lpstrFile=name;dialog.nMaxFile=32768;dialog.lpstrDefExt=L"bas";
    dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
    if(GetOpenFileNameW(&dialog))LaunchProgram(window,name);
    gPaused.store(previous);
}
void Controls(HWND window) {
    bool previous=gPaused.exchange(true);DesktopUpdateMouseCapture(window,false);
    MessageBoxW(window,
        L"MMB4W Emulator\n\n"
        L"Ctrl+O  Open a BASIC program\nCtrl+R  Reset the current program\n"
        L"F9  Pause / resume\nF10  Keyboard, mouse, gamepad, display and audio settings\n"
        L"F11 / Alt+Enter  Fullscreen\nCtrl+M  Mute / unmute audio\n"
        L"F12  Save a screenshot\nCtrl+Alt  Release a captured mouse\n\n"
        L"Keyboard and mouse input are available to MMBasic programs.\n"
        L"A connected XInput controller defaults to arrows, A for Space, B for M, Start for Enter and Back for Esc.\n\n"
        L"Use Input > Settings to change mappings and save an input profile.",
        L"Controls and shortcuts",MB_OK|MB_ICONINFORMATION);
    gPaused.store(previous);
}
void About(HWND window) {
    bool previous=gPaused.exchange(true);DesktopUpdateMouseCapture(window,false);
    MessageBoxW(window,
        L"MMB4W Emulator\nMMBasic for Windows 5.07.03b20\n\n"
        L"This product includes software developed by Geoff Graham and Peter Mather.\n"
        L"MMBasic copyright notices and redistribution terms are retained in MMBasic-LICENSE.txt.\n\n"
        L"Portable desktop controls, input profiles, screenshots and menus are included in this build.",
        L"About MMB4W Emulator",MB_OK|MB_ICONINFORMATION);
    gPaused.store(previous);
}

struct SettingsDialog {
    HWND window=nullptr, owner=nullptr;
    DesktopSettings draft;
    std::vector<HWND> pages[4];
    int page=0;
    bool capturing=false;
    bool accepted=false;
};
HWND Child(SettingsDialog& dialog,int page,const wchar_t* kind,const wchar_t* text,DWORD style,int x,int y,int width,int height,int id=0) {
    HWND control=CreateWindowExW((style&WS_BORDER)?WS_EX_CLIENTEDGE:0,kind,text,WS_CHILD|WS_VISIBLE|style,x,y,width,height,dialog.window,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
    SendMessageW(control,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);
    if(page>=0)dialog.pages[page].push_back(control);
    return control;
}
void Label(SettingsDialog& dialog,int page,const wchar_t* text,int x,int y,int width,int height=22) {Child(dialog,page,L"STATIC",text,SS_LEFT,x,y,width,height);}
HWND Combo(SettingsDialog& dialog,int page,int x,int y,int width,int id) {
    return Child(dialog,page,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,x,y,width,300,id);
}
void AddGuestKeys(HWND combo) {
    std::vector<int> keys{0,128,129,130,131,32,77,13,27,8,9,127,132,134,135,136,137};
    for(int key='A';key<='Z';++key)if(key!=77)keys.push_back(key);
    for(int key='0';key<='9';++key)keys.push_back(key);
    for(int key=145;key<=156;++key)keys.push_back(key);
    for(int key:{33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,58,59,60,61,62,63,64,91,92,93,94,95,96,123,124,125,126})keys.push_back(key);
    for(int key:keys) {
        LRESULT index=SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(KeyName(key).c_str()));
        SendMessageW(combo,CB_SETITEMDATA,index,key);
    }
}
void SelectKey(HWND combo,int key) {
    int count=static_cast<int>(SendMessageW(combo,CB_GETCOUNT,0,0));
    for(int i=0;i<count;++i)if(static_cast<int>(SendMessageW(combo,CB_GETITEMDATA,i,0))==key){SendMessageW(combo,CB_SETCURSEL,i,0);return;}
    // Preserve less common BASIC codes loaded from a hand-edited profile.
    LRESULT index=SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(KeyName(key).c_str()));
    SendMessageW(combo,CB_SETITEMDATA,index,key);SendMessageW(combo,CB_SETCURSEL,index,0);
}
int SelectedKey(HWND combo) {
    LRESULT selection=SendMessageW(combo,CB_GETCURSEL,0,0);
    return selection==CB_ERR?0:static_cast<int>(SendMessageW(combo,CB_GETITEMDATA,selection,0));
}
void Check(HWND window,int id,bool state){CheckDlgButton(window,id,state?BST_CHECKED:BST_UNCHECKED);}
bool Checked(HWND window,int id){return IsDlgButtonChecked(window,id)==BST_CHECKED;}
void EditNumber(HWND window,int id,int number){SetDlgItemTextW(window,id,Number(number).c_str());}
int ReadNumber(HWND window,int id,int fallback,int low,int high) {
    wchar_t value[40]{};GetDlgItemTextW(window,id,value,40);
    wchar_t* end=nullptr;long number=wcstol(value,&end,10);
    return end==value||*end||number<low||number>high?fallback:static_cast<int>(number);
}
void RefreshMappings(SettingsDialog& dialog) {
    HWND list=GetDlgItem(dialog.window,MappingList);
    SendMessageW(list,LB_RESETCONTENT,0,0);
    for(int key=1;key<256;++key)if(dialog.draft.keyboard[key]!=key) {
        std::wstring line=KeyName(key)+L"  ->  "+KeyName(dialog.draft.keyboard[key]);
        LRESULT item=SendMessageW(list,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(line.c_str()));
        SendMessageW(list,LB_SETITEMDATA,item,key);
    }
    if(SendMessageW(list,LB_GETCOUNT,0,0)==0) {
        LRESULT item=SendMessageW(list,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"No overrides; keyboard passes through."));
        SendMessageW(list,LB_SETITEMDATA,item,-1);
    }
}
void SetPage(SettingsDialog& dialog,int page) {
    dialog.page=page;
    for(int p=0;p<4;++p)for(HWND control:dialog.pages[p])ShowWindow(control,p==page?SW_SHOW:SW_HIDE);
    for(int p=0;p<4;++p)SendDlgItemMessageW(dialog.window,PageKeyboard+p,BM_SETSTATE,p==page,0);
    dialog.capturing=false;SetDlgItemTextW(dialog.window,CaptureKey,L"Assign a host key...");
}
void Populate(SettingsDialog& dialog) {
    HWND window=dialog.window;
    int profile=dialog.draft.profile==L"Gauntlet"?1:dialog.draft.profile==L"Custom"?2:0;
    SendDlgItemMessageW(window,ProfileChoice,CB_SETCURSEL,profile,0);
    RefreshMappings(dialog);
    Check(window,MouseEnable,dialog.draft.mouseEnabled);
    for(int i=0;i<3;++i)SelectKey(GetDlgItem(window,MouseLeft+i),dialog.draft.mouseButtonMap[i]);
    SelectKey(GetDlgItem(window,WheelUp),dialog.draft.mouseWheelUp);
    SelectKey(GetDlgItem(window,WheelDown),dialog.draft.mouseWheelDown);
    Check(window,MouseMotion,dialog.draft.mouseMotion);Check(window,MouseCapture,dialog.draft.captureMouse);
    EditNumber(window,MouseDeadZone,dialog.draft.mouseDeadZone);
    Check(window,GamepadEnable,dialog.draft.gamepadEnabled);
    EditNumber(window,GamepadDeadZone,dialog.draft.gamepadDeadZone*100/32767);
    for(int i=0;i<DesktopGamepadActions;++i)SelectKey(GetDlgItem(window,GamepadFirst+i),dialog.draft.gamepadMap[i]);
    SendDlgItemMessageW(window,ScaleChoice,CB_SETCURSEL,dialog.draft.scale-1,0);
    Check(window,FullscreenChoice,dialog.draft.fullscreen);EditNumber(window,VolumeEdit,dialog.draft.volume);Check(window,MutedChoice,dialog.draft.muted);
}
void ReadControls(SettingsDialog& dialog) {
    HWND window=dialog.window;
    dialog.draft.mouseEnabled=Checked(window,MouseEnable);
    for(int i=0;i<3;++i)dialog.draft.mouseButtonMap[i]=SelectedKey(GetDlgItem(window,MouseLeft+i));
    dialog.draft.mouseWheelUp=SelectedKey(GetDlgItem(window,WheelUp));dialog.draft.mouseWheelDown=SelectedKey(GetDlgItem(window,WheelDown));
    dialog.draft.mouseMotion=Checked(window,MouseMotion);dialog.draft.captureMouse=Checked(window,MouseCapture);
    dialog.draft.mouseDeadZone=ReadNumber(window,MouseDeadZone,24,1,200);
    dialog.draft.gamepadEnabled=Checked(window,GamepadEnable);
    dialog.draft.gamepadDeadZone=ReadNumber(window,GamepadDeadZone,30,0,90)*32767/100;
    for(int i=0;i<DesktopGamepadActions;++i)dialog.draft.gamepadMap[i]=SelectedKey(GetDlgItem(window,GamepadFirst+i));
    dialog.draft.scale=Clamp(static_cast<int>(SendDlgItemMessageW(window,ScaleChoice,CB_GETCURSEL,0,0))+1,1,4);
    dialog.draft.fullscreen=Checked(window,FullscreenChoice);dialog.draft.volume=ReadNumber(window,VolumeEdit,80,0,100);dialog.draft.muted=Checked(window,MutedChoice);
}
void CreateSettingsControls(SettingsDialog& dialog) {
    Label(dialog,-1,L"Profile:",20,22,48);
    HWND profile=Combo(dialog,-1,75,17,160,ProfileChoice);
    for(const wchar_t* name:{L"Default",L"Gauntlet",L"Custom"})SendMessageW(profile,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(name));
    Label(dialog,-1,L"Choosing a profile loads its saved input controls.",255,22,540);
    const wchar_t* pages[]={L"Keyboard",L"Mouse",L"Gamepad",L"Display and sound"};
    for(int p=0;p<4;++p)Child(dialog,-1,L"BUTTON",pages[p],BS_PUSHBUTTON|WS_TABSTOP,20+p*197,57,185,28,PageKeyboard+p);
    for(int p=0;p<4;++p)Child(dialog,p,L"BUTTON",pages[p],BS_GROUPBOX,18,98,784,430);
    Label(dialog,0,L"Custom keyboard overrides",36,124,350);
    Child(dialog,0,L"LISTBOX",L"",LBS_NOTIFY|WS_BORDER|WS_VSCROLL|WS_TABSTOP,36,149,355,285,MappingList);
    Label(dialog,0,L"Guest key / action to send:",420,125,335);
    HWND guest=Combo(dialog,0,420,150,335,GuestKeyChoice);AddGuestKeys(guest);SelectKey(guest,128);
    Child(dialog,0,L"BUTTON",L"Assign a host key...",BS_PUSHBUTTON|WS_TABSTOP,420,199,335,32,CaptureKey);
    Label(dialog,0,L"Choose the guest action, click Assign, then press one physical key. Letters are matched regardless of case.",420,246,335,62);
    Child(dialog,0,L"BUTTON",L"Remove selected override",BS_PUSHBUTTON|WS_TABSTOP,420,330,335,30,RemoveMapping);
    Label(dialog,0,L"Unmodified keys keep their normal BASIC meanings. Several host keys may send the same action. F1 and F9-F12 are reserved emulator shortcuts.",36,459,720,48);
    Child(dialog,1,L"BUTTON",L"Enable guest mouse input",BS_AUTOCHECKBOX|WS_TABSTOP,40,128,350,24,MouseEnable);
    Label(dialog,1,L"Native MOUSE() coordinates, buttons and wheel remain available to BASIC programs.",40,160,720,24);
    const wchar_t* mouseLabels[]={L"Left button",L"Right button",L"Middle button",L"Wheel up",L"Wheel down"};
    const int mouseIds[]={MouseLeft,MouseRight,MouseMiddle,WheelUp,WheelDown};
    for(int i=0;i<5;++i) {
        int x=i%2?425:40,y=207+(i/2)*52;
        Label(dialog,1,mouseLabels[i],x,y,105);
        HWND combo=Combo(dialog,1,x+115,y-4,205,mouseIds[i]);AddGuestKeys(combo);
    }
    Child(dialog,1,L"BUTTON",L"Mouse movement sends directions (center dead zone)",BS_AUTOCHECKBOX|WS_TABSTOP,40,378,490,24,MouseMotion);
    Label(dialog,1,L"Dead zone (pixels)",540,380,150);
    Child(dialog,1,L"EDIT",L"24",ES_NUMBER|WS_BORDER|WS_TABSTOP,690,376,65,24,MouseDeadZone);
    Child(dialog,1,L"BUTTON",L"Capture pointer inside the game window",BS_AUTOCHECKBOX|WS_TABSTOP,40,420,560,24,MouseCapture);
    Label(dialog,1,L"Press Ctrl+Alt to release a captured pointer. The Gauntlet profile maps left click to fire and right click to magic; mouse movement is optional.",40,464,720,46);
    Child(dialog,2,L"BUTTON",L"Enable XInput gamepad",BS_AUTOCHECKBOX|WS_TABSTOP,40,128,350,24,GamepadEnable);
    Label(dialog,2,L"Stick dead zone (%)",425,130,165);
    Child(dialog,2,L"EDIT",L"30",ES_NUMBER|WS_BORDER|WS_TABSTOP,600,126,65,24,GamepadDeadZone);
    Label(dialog,2,L"Uses the first connected XInput controller. D-pad and left stick share direction mappings.",40,163,720,24);
    const wchar_t* gamepadNames[]={L"Up",L"Down",L"Left",L"Right",L"A button",L"B button",L"X button",L"Y button",L"Start",L"Back"};
    for(int i=0;i<DesktopGamepadActions;++i) {
        int x=i<5?40:425,y=212+(i%5)*54;
        Label(dialog,2,gamepadNames[i],x,y,100);
        HWND combo=Combo(dialog,2,x+115,y-4,205,GamepadFirst+i);AddGuestKeys(combo);
    }
    Label(dialog,3,L"Preferred integer scale",40,135,180);
    HWND scale=Combo(dialog,3,235,130,180,ScaleChoice);
    for(const wchar_t* text:{L"1x",L"2x",L"3x",L"4x"})SendMessageW(scale,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));
    Child(dialog,3,L"BUTTON",L"Fullscreen (F11 / Alt+Enter)",BS_AUTOCHECKBOX|WS_TABSTOP,40,184,440,24,FullscreenChoice);
    Label(dialog,3,L"Master volume (%)",40,239,180);
    Child(dialog,3,L"EDIT",L"80",ES_NUMBER|WS_BORDER|WS_TABSTOP,235,234,80,24,VolumeEdit);
    Child(dialog,3,L"BUTTON",L"Mute audio",BS_AUTOCHECKBOX|WS_TABSTOP,40,282,440,24,MutedChoice);
    Label(dialog,3,L"Scaling and fullscreen resize only the desktop window; they do not restart BASIC or clear game graphics. Larger scales are limited to the available monitor area.\n\nF9 pauses / resumes the program. Ctrl+R resets it. F12 saves a BMP screenshot in Screenshots beside the emulator.\n\nSettings and profiles are saved in emulator.ini beside the executable.",40,340,720,150);
    Label(dialog,-1,L"Apply saves the selected profile. Save as Custom keeps a separate editable copy.",22,535,760,22);
    Child(dialog,-1,L"BUTTON",L"Restore profile defaults",BS_PUSHBUTTON|WS_TABSTOP,20,567,185,28,RestoreDefaults);
    Child(dialog,-1,L"BUTTON",L"Save as Custom",BS_PUSHBUTTON|WS_TABSTOP,220,567,160,28,SaveCustom);
    Child(dialog,-1,L"BUTTON",L"Apply",BS_PUSHBUTTON|WS_TABSTOP,508,567,90,28,ApplySettings);
    Child(dialog,-1,L"BUTTON",L"OK",BS_DEFPUSHBUTTON|WS_TABSTOP,610,567,90,28,AcceptSettings);
    Child(dialog,-1,L"BUTTON",L"Cancel",BS_PUSHBUTTON|WS_TABSTOP,712,567,90,28,CancelSettings);
    Populate(dialog);SetPage(dialog,0);
}
LRESULT CALLBACK SettingsProc(HWND window,UINT message,WPARAM wParam,LPARAM lParam) {
    SettingsDialog* dialog=reinterpret_cast<SettingsDialog*>(GetWindowLongPtrW(window,GWLP_USERDATA));
    if(message==WM_NCCREATE) {
        dialog=static_cast<SettingsDialog*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
        dialog->window=window;SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(dialog));
    }
    if(!dialog)return DefWindowProcW(window,message,wParam,lParam);
    switch(message) {
    case WM_CREATE:CreateSettingsControls(*dialog);return 0;
    case WM_COMMAND:{
        int id=LOWORD(wParam);
        if(id>=PageKeyboard && id<=PageGeneral){SetPage(*dialog,id-PageKeyboard);return 0;}
        if(id==ProfileChoice && HIWORD(wParam)==CBN_SELCHANGE) {
            const wchar_t* profiles[]={L"Default",L"Gauntlet",L"Custom"};
            int selected=static_cast<int>(SendDlgItemMessageW(window,ProfileChoice,CB_GETCURSEL,0,0));
            DesktopSettings loaded=ReadProfile(profiles[Clamp(selected,0,2)]);CopyDisplay(dialog->draft,loaded);dialog->draft=loaded;Populate(*dialog);return 0;
        }
        if(id==CaptureKey){dialog->capturing=true;SetDlgItemTextW(window,CaptureKey,L"Press a host key now (Esc cancels)...");SetFocus(window);return 0;}
        if(id==RemoveMapping) {
            HWND list=GetDlgItem(window,MappingList);LRESULT row=SendMessageW(list,LB_GETCURSEL,0,0);
            if(row!=LB_ERR){LRESULT source=SendMessageW(list,LB_GETITEMDATA,row,0);if(source>0&&source<256)dialog->draft.keyboard[source]=static_cast<int>(source);}
            RefreshMappings(*dialog);return 0;
        }
        if(id==RestoreDefaults) {
            DesktopSettings defaults=Defaults(dialog->draft.profile);CopyDisplay(dialog->draft,defaults);dialog->draft=defaults;Populate(*dialog);return 0;
        }
        if(id==ApplySettings || id==AcceptSettings || id==SaveCustom) {
            ReadControls(*dialog);
            if(id==SaveCustom)dialog->draft.profile=L"Custom";
            Apply(dialog->draft);Populate(*dialog);
            if(id==AcceptSettings){dialog->accepted=true;DestroyWindow(window);}return 0;
        }
        if(id==CancelSettings || id==IDCANCEL){DestroyWindow(window);return 0;}
        break;
    }
    case WM_CLOSE:DestroyWindow(window);return 0;
    case WM_DESTROY:gDialog=nullptr;return 0;
    }
    return DefWindowProcW(window,message,wParam,lParam);
}
void ShowSettings(HWND owner) {
    if(gDialog){SetForegroundWindow(gDialog);return;}
    bool previous=gPaused.exchange(true);DesktopUpdateMouseCapture(owner,false);
    SettingsDialog dialog;dialog.owner=owner;dialog.draft=DesktopSettingsSnapshot();
    WNDCLASSW cls{};cls.lpfnWndProc=SettingsProc;cls.hInstance=GetModuleHandleW(nullptr);
    cls.hCursor=LoadCursor(nullptr,IDC_ARROW);cls.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1);cls.lpszClassName=L"MMB4W_DESKTOP_SETTINGS";
    RegisterClassW(&cls);
    RECT rectangle{0,0,820,612};AdjustWindowRectEx(&rectangle,WS_CAPTION|WS_SYSMENU|WS_POPUP,FALSE,WS_EX_DLGMODALFRAME);
    RECT parent{};GetWindowRect(owner,&parent);
    int width=rectangle.right-rectangle.left,height=rectangle.bottom-rectangle.top;
    int x=std::max(0,static_cast<int>(parent.left+(parent.right-parent.left-width)/2));
    int y=std::max(0,static_cast<int>(parent.top+(parent.bottom-parent.top-height)/2));
    gDialog=CreateWindowExW(WS_EX_DLGMODALFRAME,L"MMB4W_DESKTOP_SETTINGS",L"MMB4W Emulator settings",WS_CAPTION|WS_SYSMENU|WS_POPUP,x,y,width,height,owner,nullptr,GetModuleHandleW(nullptr),&dialog);
    if(!gDialog){gPaused.store(previous);return;}
    EnableWindow(owner,FALSE);ShowWindow(gDialog,SW_SHOW);SetForegroundWindow(gDialog);
    MSG message{};
    while(gDialog && GetMessageW(&message,nullptr,0,0)>0) {
        if(dialog.capturing && (message.message==WM_KEYDOWN || message.message==WM_SYSKEYDOWN)) {
            if(message.wParam==VK_ESCAPE){dialog.capturing=false;SetDlgItemTextW(gDialog,CaptureKey,L"Assign a host key...");continue;}
            int source=VkToBasic(message.wParam);
            if(source<1)continue;
            if(source==145 || (source>=153 && source<=156)) {
                MessageBoxW(gDialog,L"F1 and F9-F12 are reserved emulator shortcuts. Choose a different host key.",L"Keyboard mapping",MB_OK|MB_ICONINFORMATION);continue;
            }
            int target=SelectedKey(GetDlgItem(gDialog,GuestKeyChoice));dialog.draft.keyboard[source]=target;
            dialog.capturing=false;RefreshMappings(dialog);SetDlgItemTextW(gDialog,CaptureKey,(L"Assigned "+KeyName(source)+L" -> "+KeyName(target)).c_str());continue;
        }
        if(!IsDialogMessageW(gDialog,&message)){TranslateMessage(&message);DispatchMessageW(&message);}
    }
    EnableWindow(owner,TRUE);SetForegroundWindow(owner);SetFocus(owner);gPaused.store(previous);UpdateMenu(owner);
}
void MenuCommand(HWND window,int command) {
    DesktopSettings settings=DesktopSettingsSnapshot();
    if(command>=RecentFirst && command<RecentFirst+5) {
        std::wstring path;
        {std::lock_guard<std::mutex> lock(gFileMutex);size_t index=command-RecentFirst;if(index<gRecent.size())path=gRecent[index];}
        if(!path.empty())LaunchProgram(window,path);return;
    }
    if(command>=Scale1 && command<=Scale4){settings.scale=command-Scale1+1;Apply(settings);return;}
    if(command>=Volume0 && command<=Volume100){settings.volume=(command-Volume0)*25;settings.muted=false;Apply(settings);return;}
    if(command>=ProfileDefault && command<=ProfileCustom) {
        const wchar_t* names[]={L"Default",L"Gauntlet",L"Custom"};DesktopSettings loaded=ReadProfile(names[command-ProfileDefault]);CopyDisplay(settings,loaded);Apply(loaded);return;
    }
    switch(command) {
    case OpenProgram:OpenDialog(window);break;
    case ResetProgram:{std::wstring program=DesktopCurrentProgram();if(program.empty())OpenDialog(window);else LaunchProgram(window,program);break;}
    case ExitApplication:gQuit.store(true);break;
    case PauseEmulation:gPaused.store(!gPaused.load());UpdateMenu(window);break;
    case InputSettings:ShowSettings(window);break;
    case Screenshot:gScreenshot.store(true);break;
    case Fullscreen:settings.fullscreen=!settings.fullscreen;Apply(settings);break;
    case MuteAudio:settings.muted=!settings.muted;Apply(settings);break;
    case CaptureMouse:settings.captureMouse=!settings.captureMouse;Apply(settings);break;
    case ControlsHelp:Controls(window);break;
    case AboutEmulator:About(window);break;
    }
}
HMENU BuildMenu() {
    HMENU bar=CreateMenu(),file=CreatePopupMenu(),emulation=CreatePopupMenu(),input=CreatePopupMenu(),display=CreatePopupMenu(),audio=CreatePopupMenu(),help=CreatePopupMenu();
    AppendMenuW(file,MF_STRING,OpenProgram,L"&Open program...\tCtrl+O");
    HMENU recent=CreatePopupMenu();
    {std::lock_guard<std::mutex> lock(gFileMutex);
        if(gRecent.empty())AppendMenuW(recent,MF_STRING|MF_GRAYED,0,L"No recent programs");
        for(size_t i=0;i<gRecent.size();++i)AppendMenuW(recent,MF_STRING,RecentFirst+i,gRecent[i].c_str());
    }
    AppendMenuW(file,MF_POPUP,reinterpret_cast<UINT_PTR>(recent),L"&Recent programs");
    AppendMenuW(file,MF_SEPARATOR,0,nullptr);AppendMenuW(file,MF_STRING,ResetProgram,L"&Reset current program\tCtrl+R");AppendMenuW(file,MF_STRING,ExitApplication,L"E&xit");
    AppendMenuW(emulation,MF_STRING,PauseEmulation,L"&Pause\tF9");AppendMenuW(emulation,MF_STRING,Screenshot,L"Save &screenshot\tF12");
    AppendMenuW(input,MF_STRING,InputSettings,L"&Settings...\tF10");AppendMenuW(input,MF_SEPARATOR,0,nullptr);
    AppendMenuW(input,MF_STRING,ProfileDefault,L"&Default profile");AppendMenuW(input,MF_STRING,ProfileGauntlet,L"&Gauntlet profile");AppendMenuW(input,MF_STRING,ProfileCustom,L"&Custom profile");
    AppendMenuW(input,MF_SEPARATOR,0,nullptr);AppendMenuW(input,MF_STRING,CaptureMouse,L"&Capture mouse (Ctrl+Alt releases)");
    for(int i=0;i<4;++i)AppendMenuW(display,MF_STRING,Scale1+i,(Number(i+1)+L"x scale").c_str());
    AppendMenuW(display,MF_SEPARATOR,0,nullptr);AppendMenuW(display,MF_STRING,Fullscreen,L"&Fullscreen\tF11 / Alt+Enter");
    AppendMenuW(audio,MF_STRING,MuteAudio,L"&Mute\tCtrl+M");AppendMenuW(audio,MF_SEPARATOR,0,nullptr);
    for(int i=0;i<5;++i)AppendMenuW(audio,MF_STRING,Volume0+i,(Number(i*25)+L"% volume").c_str());
    AppendMenuW(audio,MF_SEPARATOR,0,nullptr);AppendMenuW(audio,MF_STRING,InputSettings,L"Volume / input settings...");
    AppendMenuW(help,MF_STRING,ControlsHelp,L"&Controls and shortcuts\tF1");AppendMenuW(help,MF_STRING,AboutEmulator,L"&About MMB4W Emulator");
    AppendMenuW(bar,MF_POPUP,reinterpret_cast<UINT_PTR>(file),L"&File");AppendMenuW(bar,MF_POPUP,reinterpret_cast<UINT_PTR>(emulation),L"&Emulation");
    AppendMenuW(bar,MF_POPUP,reinterpret_cast<UINT_PTR>(input),L"&Input");AppendMenuW(bar,MF_POPUP,reinterpret_cast<UINT_PTR>(display),L"&Display");
    AppendMenuW(bar,MF_POPUP,reinterpret_cast<UINT_PTR>(audio),L"&Audio");AppendMenuW(bar,MF_POPUP,reinterpret_cast<UINT_PTR>(help),L"&Help");
    return bar;
}
void UpdateMenu(HWND window) {
    if(!IsWindow(window))return;
    HMENU menu=GetMenu(window);if(!menu)return;
    DesktopSettings settings=DesktopSettingsSnapshot();
    CheckMenuItem(menu,PauseEmulation,MF_BYCOMMAND|(gPaused.load()?MF_CHECKED:MF_UNCHECKED));
    ModifyMenuW(menu,PauseEmulation,MF_BYCOMMAND|MF_STRING,PauseEmulation,gPaused.load()?L"&Resume\tF9":L"&Pause\tF9");
    CheckMenuItem(menu,Fullscreen,MF_BYCOMMAND|(settings.fullscreen?MF_CHECKED:MF_UNCHECKED));
    CheckMenuItem(menu,MuteAudio,MF_BYCOMMAND|(settings.muted?MF_CHECKED:MF_UNCHECKED));
    CheckMenuItem(menu,CaptureMouse,MF_BYCOMMAND|(settings.captureMouse&&!gCaptureReleased.load()?MF_CHECKED:MF_UNCHECKED));
    CheckMenuRadioItem(menu,Scale1,Scale4,Scale1+settings.scale-1,MF_BYCOMMAND);
    int profile=settings.profile==L"Gauntlet"?ProfileGauntlet:settings.profile==L"Custom"?ProfileCustom:ProfileDefault;
    CheckMenuRadioItem(menu,ProfileDefault,ProfileCustom,profile,MF_BYCOMMAND);
    for(int i=0;i<5;++i)CheckMenuItem(menu,Volume0+i,MF_BYCOMMAND|(settings.volume==i*25?MF_CHECKED:MF_UNCHECKED));
    DrawMenuBar(window);
}
bool WriteBitmap(const std::wstring& path,const uint32_t* pixels,int width,int height) {
    if(!pixels||width<=0||height<=0||width>8192||height>8192)return false;
    FILE* file=nullptr;_wfopen_s(&file,path.c_str(),L"wb");if(!file)return false;
    DWORD stride=static_cast<DWORD>((width*3+3)&~3),imageSize=stride*height;
    BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);header.bfSize=header.bfOffBits+imageSize;
    BITMAPINFOHEADER info{};info.biSize=sizeof(info);info.biWidth=width;info.biHeight=height;info.biPlanes=1;info.biBitCount=24;info.biCompression=BI_RGB;info.biSizeImage=imageSize;
    bool saved=fwrite(&header,1,sizeof(header),file)==sizeof(header)&&fwrite(&info,1,sizeof(info),file)==sizeof(info);
    std::vector<unsigned char> row(stride,0);
    for(int y=height-1;y>=0&&saved;--y) {
        // olc::Pixel.n stores red in the low byte; BMP rows use B, G, R.
        for(int x=0;x<width;++x){uint32_t color=pixels[y*width+x];row[x*3]=static_cast<unsigned char>(color>>16);row[x*3+1]=static_cast<unsigned char>(color>>8);row[x*3+2]=static_cast<unsigned char>(color);}
        saved=fwrite(row.data(),1,stride,file)==stride;
    }
    return fclose(file)==0&&saved;
}
}

void DesktopInitialize() {
    std::call_once(gInitialize,[]{
        wchar_t path[32768]{};GetModuleFileNameW(nullptr,path,32768);gExecutable=path;gDirectory=Parent(gExecutable);gIni=gDirectory+L"\\emulator.ini";
        int count=0;wchar_t** arguments=CommandLineToArgvW(GetCommandLineW(),&count);
        if(arguments){gNoAudio=count>1&&wcscmp(arguments[1],L"0")==0;LocalFree(arguments);}
        std::wstring profile=IniString(L"Emulator",L"Profile",L"Default");if(!ValidProfile(profile))profile=L"Default";
        gSettings=ReadProfile(profile);gSettings.scale=IniInt(L"Display",L"Scale",3,1,4);gSettings.fullscreen=IniInt(L"Display",L"Fullscreen",0,0,1)!=0;
        gSettings.volume=IniInt(L"Audio",L"Volume",80,0,100);gSettings.muted=IniInt(L"Audio",L"Muted",0,0,1)!=0;
        gVolume.store(gSettings.muted?0.0f:gSettings.volume/100.0f);
        for(int i=0;i<5;++i){std::wstring recent=IniString(L"Recent",Number(i));if(!recent.empty()&&IsFile(recent))gRecent.push_back(recent);}
    });
}
void DesktopAttachWindow(HWND window) {
    DesktopInitialize();gWindow.store(window);
    HMENU old=GetMenu(window),menu=BuildMenu();SetMenu(window,menu);if(old)DestroyMenu(old);
    UpdateMenu(window);gDisplayChange.store(true);
}
HWND DesktopWindowHandle(){return gWindow.load();}
DesktopSettings DesktopSettingsSnapshot(){DesktopInitialize();std::lock_guard<std::mutex> lock(gSettingsMutex);return gSettings;}
uint64_t DesktopSettingsRevision(){return gRevision.load();}
void DesktopSetProgram(const char* path) {
    if(!path)return;
    int count=MultiByteToWideChar(CP_ACP,0,path,-1,nullptr,0);if(count<=0)return;
    std::vector<wchar_t> wide(count);MultiByteToWideChar(CP_ACP,0,path,-1,wide.data(),count);DesktopSetProgram(wide.data());
}
void DesktopSetProgram(const wchar_t* path) {
    DesktopInitialize();if(!path||!*path)return;
    std::wstring program=path;size_t comma=program.find(L',');if(comma!=std::wstring::npos)program.resize(comma);
    if(program.size()>1&&program.front()==L'"'&&program.back()==L'"')program=program.substr(1,program.size()-2);
    wchar_t absolute[32768]{};if(GetFullPathNameW(program.c_str(),32768,absolute,nullptr))program=absolute;
    {std::lock_guard<std::mutex> lock(gProgramMutex);gProgram=program;}
    AddRecent(program);
}
std::wstring DesktopCurrentProgram(){std::lock_guard<std::mutex> lock(gProgramMutex);return gProgram;}
bool DesktopIsPaused(){return gPaused.load();}
float DesktopVolume(){return gVolume.load();}
bool DesktopQuitRequested(){return gQuit.load();}
bool DesktopConsumeScreenshot(){return gScreenshot.exchange(false);}
bool DesktopConsumeDisplayRequest(int& scale,bool& fullscreen) {
    if(!gDisplayChange.exchange(false))return false;
    DesktopSettings settings=DesktopSettingsSnapshot();scale=settings.scale;fullscreen=settings.fullscreen;return true;
}
void DesktopUpdateMouseCapture(HWND window,bool focused) {
    DesktopSettings settings=DesktopSettingsSnapshot();
    bool capture=settings.captureMouse&&!gCaptureReleased.load()&&!gInMenu.load()&&focused&&!DesktopIsPaused()&&GetForegroundWindow()==window;
    if(capture) {
        RECT rectangle{};GetClientRect(window,&rectangle);POINT start{rectangle.left,rectangle.top},end{rectangle.right,rectangle.bottom};ClientToScreen(window,&start);ClientToScreen(window,&end);
        RECT screen{start.x,start.y,end.x,end.y};ClipCursor(&screen);
    } else ClipCursor(nullptr);
}
bool DesktopHandleMessage(HWND window,UINT message,WPARAM wParam,LPARAM lParam,LRESULT& result) {
    if(message==WM_APP+414){UpdateMenu(window);result=0;return true;}
    if(message==WM_APP+415){HMENU old=GetMenu(window);SetMenu(window,BuildMenu());if(old)DestroyMenu(old);UpdateMenu(window);result=0;return true;}
    if(message==WM_INITMENU){UpdateMenu(window);return false;}
    if(message==WM_COMMAND && LOWORD(wParam)>=OpenProgram && LOWORD(wParam)<RecentFirst+5){MenuCommand(window,LOWORD(wParam));result=0;return true;}
    if(message==WM_ENTERMENULOOP){gInMenu.store(true);DesktopUpdateMouseCapture(window,false);return false;}
    if(message==WM_EXITMENULOOP){gInMenu.store(false);return false;}
    if(message==WM_KILLFOCUS){DesktopUpdateMouseCapture(window,false);return false;}
    if(message==WM_DESTROY){if(gWindow.load()==window)gWindow.store(nullptr);ClipCursor(nullptr);return false;}
    if(message==WM_SETCURSOR && LOWORD(lParam)==HTCLIENT) {
        DesktopSettings settings=DesktopSettingsSnapshot();
        if(settings.captureMouse&&!gCaptureReleased.load()&&!gPaused.load()){SetCursor(nullptr);result=TRUE;return true;}
    }
    if((message==WM_KEYUP || message==WM_SYSKEYUP) && wParam<256 && gHotkeyDown[wParam]){gHotkeyDown[wParam]=false;result=0;return true;}
    if(message==WM_KEYDOWN || message==WM_SYSKEYDOWN) {
        bool control=(GetKeyState(VK_CONTROL)&0x8000)!=0,alt=(GetKeyState(VK_MENU)&0x8000)!=0;
        if(control&&alt){gCaptureReleased.store(true);DesktopUpdateMouseCapture(window,false);UpdateMenu(window);}
        int command=0;
        if(control&&wParam=='O')command=OpenProgram;
        else if(control&&wParam=='R')command=ResetProgram;
        else if(control&&wParam=='M')command=MuteAudio;
        else if(wParam==VK_F1)command=ControlsHelp;
        else if(wParam==VK_F9)command=PauseEmulation;
        else if(wParam==VK_F10)command=InputSettings;
        else if(wParam==VK_F11)command=Fullscreen;
        else if(wParam==VK_F12)command=Screenshot;
        else if(alt&&wParam==VK_RETURN)command=Fullscreen;
        if(command) {
            if(!(lParam&(1L<<30))){if(wParam<256)gHotkeyDown[wParam]=true;MenuCommand(window,command);}
            result=0;return true;
        }
    }
    return false;
}
void DesktopSaveScreenshot(const uint32_t* pixels,int width,int height) {
    DesktopInitialize();if(!pixels||width<=0||height<=0||width>8192||height>8192)return;
    std::wstring directory=gDirectory+L"\\Screenshots";CreateDirectoryW(directory.c_str(),nullptr);
    SYSTEMTIME time{};GetLocalTime(&time);wchar_t name[100]{};
    swprintf_s(name,L"\\MMB4W-%04d%02d%02d-%02d%02d%02d-%03d.bmp",time.wYear,time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond,time.wMilliseconds);
    std::wstring path=directory+name;
    bool saved=WriteBitmap(path,pixels,width,height);
    bool previous=gPaused.exchange(true);DesktopUpdateMouseCapture(gWindow.load(),false);
    MessageBoxW(gWindow.load(),saved?(L"Screenshot saved:\n\n"+path).c_str():L"Windows could not finish saving the screenshot.",L"Save screenshot",MB_OK|(saved?MB_ICONINFORMATION:MB_ICONERROR));
    gPaused.store(previous);
}
