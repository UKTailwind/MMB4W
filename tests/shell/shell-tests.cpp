// Standalone Win32 contract checks, independent of the BASIC interpreter.
#include "../../DesktopShell.cpp"
#include <cstdlib>
#include <iostream>

int checks=0;
void CheckResult(bool result,const char* description) {
    ++checks;
    if(!result){std::cerr<<"FAILED: "<<description<<"\n";std::exit(1);}
}
#define CHECK(expression) CheckResult((expression),#expression)

int main() {
    DesktopInitialize();
    gIni=gDirectory+L"\\shell-test-settings.ini";
    DeleteFileW(gIni.c_str());
    DesktopSettings defaults=Defaults(L"Default");
    for(int key=0;key<256;++key)CHECK(defaults.keyboard[key]==key);
    CHECK((defaults.mouseButtonMap==std::array<int,3>({0,0,0})));
    CHECK((Defaults(L"Gauntlet").mouseButtonMap==std::array<int,3>({32,77,0})));
    CHECK(defaults.gamepadMap[0]==128 && defaults.gamepadMap[4]==32);
    CHECK(Defaults(L"invalid").profile==L"Default");
    CHECK(VkToBasic(VK_UP)==128 && VkToBasic(VK_DOWN)==129);
    CHECK(VkToBasic(VK_LEFT)==130 && VkToBasic(VK_RIGHT)==131);
    CHECK(VkToBasic(VK_RETURN)==13 && VkToBasic('W')=='W');
    CHECK(VkToBasic(VK_F11)==155 && VkToBasic(VK_NUMPAD7)=='7');
    CHECK(Quote(L"C:\\a b\\file.bas")==L"\"C:\\a b\\file.bas\"");
    CHECK(Quote(L"C:\\folder\\")==L"\"C:\\folder\\\\\"");
    CHECK(Quote(L"a\"b")==L"\"a\\\"b\"");

    WriteIni(L"Profile.Custom",L"Key65",L"999");
    WriteIni(L"Profile.Custom",L"Key66",L"0");
    WriteIni(L"Profile.Custom",L"MouseDeadZone",L"-1");
    WriteIni(L"Profile.Custom",L"GamepadDeadZone",L"oops");
    DesktopSettings custom=ReadProfile(L"Custom");
    CHECK(custom.keyboard['A']=='A' && custom.keyboard['B']==0);
    CHECK(custom.mouseDeadZone==24 && custom.gamepadDeadZone==10000);
    custom.keyboard['W']=128;custom.mouseButtonMap={{32,77,13}};
    custom.mouseEnabled=false;custom.mouseWheelUp=32;custom.mouseWheelDown=77;
    custom.mouseMotion=true;custom.captureMouse=true;custom.mouseDeadZone=37;
    custom.gamepadEnabled=false;custom.gamepadMap[6]='F';custom.gamepadDeadZone=12345;
    custom.scale=2;custom.fullscreen=true;custom.volume=43;custom.muted=false;
    SaveSettings(custom);
    DesktopSettings reread=ReadProfile(L"Custom");
    CHECK(reread.keyboard==custom.keyboard && reread.mouseButtonMap==custom.mouseButtonMap);
    CHECK(!reread.mouseEnabled && reread.mouseWheelUp==32 && reread.mouseWheelDown==77);
    CHECK(reread.mouseMotion && reread.captureMouse && reread.mouseDeadZone==37);
    CHECK(!reread.gamepadEnabled && reread.gamepadMap==custom.gamepadMap && reread.gamepadDeadZone==12345);
    CHECK(IniInt(L"Display",L"Scale",1,1,4)==2 && IniInt(L"Audio",L"Volume",80,0,100)==43);
    uint64_t revision=DesktopSettingsRevision();Apply(custom);
    CHECK(DesktopSettingsRevision()==revision+1);
    CHECK(DesktopVolume()>0.429f && DesktopVolume()<0.431f);
    int scale=0;bool fullscreen=false;
    CHECK(DesktopConsumeDisplayRequest(scale,fullscreen) && scale==2 && fullscreen);
    CHECK(!DesktopConsumeDisplayRequest(scale,fullscreen));
    custom.muted=true;Apply(custom);CHECK(DesktopVolume()==0.0f);
    CHECK(!DesktopConsumeDisplayRequest(scale,fullscreen));

    WNDCLASSW cls{};cls.lpfnWndProc=DefWindowProcW;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"MMB4W_SHELL_TEST";
    RegisterClassW(&cls);
    HWND owner=CreateWindowW(cls.lpszClassName,L"Shell test",WS_OVERLAPPEDWINDOW,0,0,900,700,nullptr,nullptr,cls.hInstance,nullptr);
    CHECK(owner!=nullptr);DesktopAttachWindow(owner);
    CHECK(DesktopWindowHandle()==owner && GetMenuItemCount(GetMenu(owner))==6);
    CHECK(DesktopConsumeDisplayRequest(scale,fullscreen));
    LRESULT result=0;
    CHECK(DesktopHandleMessage(owner,WM_KEYDOWN,VK_F9,0,result) && DesktopIsPaused());
    CHECK(DesktopHandleMessage(owner,WM_KEYDOWN,VK_F9,1L<<30,result) && DesktopIsPaused());
    CHECK(DesktopHandleMessage(owner,WM_KEYUP,VK_F9,0,result));
    CHECK(DesktopHandleMessage(owner,WM_KEYDOWN,VK_F9,0,result) && !DesktopIsPaused());
    DesktopHandleMessage(owner,WM_KEYUP,VK_F9,0,result);
    CHECK(DesktopHandleMessage(owner,WM_KEYDOWN,VK_F11,0,result));
    CHECK(DesktopConsumeDisplayRequest(scale,fullscreen) && !fullscreen);
    DesktopHandleMessage(owner,WM_KEYUP,VK_F11,0,result);
    CHECK(DesktopHandleMessage(owner,WM_KEYDOWN,VK_F12,0,result));
    CHECK(DesktopConsumeScreenshot() && !DesktopConsumeScreenshot());
    DesktopHandleMessage(owner,WM_KEYUP,VK_F12,0,result);
    DesktopHandleMessage(owner,WM_ENTERMENULOOP,0,0,result);CHECK(gInMenu.load());
    DesktopHandleMessage(owner,WM_EXITMENULOOP,0,0,result);CHECK(!gInMenu.load());

    SettingsDialog dialog;dialog.draft=Defaults(L"Gauntlet");
    WNDCLASSW dialogClass{};dialogClass.lpfnWndProc=SettingsProc;dialogClass.hInstance=cls.hInstance;dialogClass.lpszClassName=L"MMB4W_SETTINGS_TEST";
    RegisterClassW(&dialogClass);
    HWND settingsWindow=CreateWindowW(dialogClass.lpszClassName,L"Settings test",WS_OVERLAPPEDWINDOW,0,0,850,700,owner,nullptr,cls.hInstance,&dialog);
    CHECK(settingsWindow!=nullptr && GetDlgItem(settingsWindow,MappingList)!=nullptr);
    CHECK(SelectedKey(GetDlgItem(settingsWindow,MouseLeft))==32);
    CHECK(SelectedKey(GetDlgItem(settingsWindow,MouseRight))==77);
    CHECK(SelectedKey(GetDlgItem(settingsWindow,GamepadFirst))==128);
    SelectKey(GetDlgItem(settingsWindow,WheelUp),13);
    Check(settingsWindow,MouseMotion,true);EditNumber(settingsWindow,VolumeEdit,67);
    ReadControls(dialog);
    CHECK(dialog.draft.mouseWheelUp==13 && dialog.draft.mouseMotion && dialog.draft.volume==67);
    SetPage(dialog,2);
    CHECK((GetWindowLongW(GetDlgItem(settingsWindow,GamepadEnable),GWL_STYLE)&WS_VISIBLE)!=0);
    CHECK((GetWindowLongW(GetDlgItem(settingsWindow,MouseEnable),GWL_STYLE)&WS_VISIBLE)==0);
    SelectKey(GetDlgItem(settingsWindow,GuestKeyChoice),255);
    CHECK(SelectedKey(GetDlgItem(settingsWindow,GuestKeyChoice))==255);
    dialog.draft.keyboard['W']=128;RefreshMappings(dialog);
    CHECK(SendDlgItemMessageW(settingsWindow,MappingList,LB_GETCOUNT,0,0)==1);
    SendDlgItemMessageW(settingsWindow,MappingList,LB_SETCURSEL,0,0);
    SendMessageW(settingsWindow,WM_COMMAND,RemoveMapping,0);
    CHECK(dialog.draft.keyboard['W']=='W');
    DestroyWindow(settingsWindow);

    const uint32_t pixels[]={0xff0000ff,0xff00ff00,0xffff0000,0xffffffff};
    std::wstring screenshot=gDirectory+L"\\shell-test-screenshot.bmp";
    CHECK(WriteBitmap(screenshot,pixels,2,2));
    CHECK(!WriteBitmap(screenshot,nullptr,2,2) && !WriteBitmap(screenshot,pixels,0,2));
    FILE* file=nullptr;_wfopen_s(&file,screenshot.c_str(),L"rb");CHECK(file!=nullptr);
    unsigned char image[70]{};CHECK(fread(image,1,sizeof(image),file)==sizeof(image));fclose(file);
    CHECK(image[0]=='B' && image[1]=='M' && image[10]==54 && image[28]==24);
    CHECK(image[54]==255 && image[55]==0 && image[56]==0); // bottom-left blue
    CHECK(image[62]==0 && image[63]==0 && image[64]==255); // top-left red
    CHECK(image[60]==0 && image[61]==0 && image[68]==0 && image[69]==0); // row padding
    for(int i=0;i<7;++i)AddRecent(gDirectory+L"\\program"+Number(i)+L".bas");
    CHECK(gRecent.size()==5 && gRecent.front().find(L"program6.bas")!=std::wstring::npos);
    std::wstring duplicate=gRecent[2];AddRecent(duplicate);CHECK(gRecent.size()==5 && gRecent.front().find(L"program4.bas")!=std::wstring::npos);
    DesktopHandleMessage(owner,WM_DESTROY,0,0,result);DestroyWindow(owner);
    DeleteFileW(gIni.c_str());DeleteFileW(screenshot.c_str());
    std::cout<<checks<<" shell checks passed\n";
}
