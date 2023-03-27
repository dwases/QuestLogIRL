// Dear ImGui: standalone example application for DirectX 11
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>

#include <string>
#include <fstream>
#include <iostream>
#include <windows.h>
#include <vector>
#include <stdio.h>
#include <cstdlib>
#include <regex>

// Data
static ID3D11Device*            g_pd3dDevice = NULL;
static ID3D11DeviceContext*     g_pd3dDeviceContext = NULL;
static IDXGISwapChain*          g_pSwapChain = NULL;
static ID3D11RenderTargetView*  g_mainRenderTargetView = NULL;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Forward declarations of custom functions
bool ArePasswordsSame(char p1[], char p2[]);
bool UserAlreadyExists(char username[], char password[]);
void AddUser(char username[], char password[]);
bool IsUserInDatabase(char username[], char password[]);
void ShowQuestlist(std::string category, char username[], std::vector <std::string>& QuestNames, std::string& activeQuest);
std::vector <std::string> UpdateQuests();
void RefreshCommand();
std::string GetCategoryFromFile(std::string name);
bool IsOwnerOK(char username[], std::string file_name);
void MarkActiveAs(std::string active_quest, std::string category);
void DisplayContents(std::string active_quest);
void AddNewQuest(char new_quest_owner[], char new_quest_name[], int item_current_idx, char new_quest_contents[]);

// Forward declarations of custom classes
class Quest;
class QuestWithLocation;

// Main code
int main(int, char**)
{
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"ImGui Example", NULL };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX11 Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // current state
    bool check_main = true, check_side = true, check_complete = false, check_failed = false;
    bool passwords_mismatch_again = false;
    bool show_login_window = true;
    bool show_register_window = false;
    bool show_journal_window = false;
    bool show_add_quest_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);\
    char login_username[64] = "";
    char new_quest_name[128] =  "";
    char new_quest_type[128] = "";
    char new_quest_contents[2048 * 8];
    char* items[] = { "Main", "Side" };
    int item_current_idx = 0;
    
    std::vector < Quest > AllQuests;
    std::vector < std::string > QuestNames;
    std::string active_quest = " ";

    //std::string new_quest_name;
    //std::string new_quest_type;

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // the login window
        if (show_login_window)
        {
            ImGui::SetNextWindowSize(ImVec2(400, 200));
            ImGui::Begin("WELCOME TO YOUR OWN IRL QUEST JOURNAL!", &show_login_window);
            ImGui::Text("Log in:");
            ImGui::Spacing();
            ImGui::InputText("(Username)", login_username, 64);
            ImGui::Spacing();
            static char login_password[64] = "";
            ImGui::InputTextWithHint("(Password)", "", login_password, IM_ARRAYSIZE(login_password), ImGuiInputTextFlags_Password);
            ImGui::Spacing();
            if (ImGui::Button("ENTER")) {
                if (IsUserInDatabase(login_username, login_password))
                {
                    show_journal_window = true;
                    show_login_window = false;
                }
            }
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Don't yet have an account of your own?");
            ImGui::Text("Click this button to");
            ImGui::SameLine();  if (ImGui::SmallButton("REGISTER"))   show_register_window = true;
            ImGui::End();
        }

        //the register window
        if (show_register_window)
        {
            ImGui::SetNextWindowSize(ImVec2(500,200));
            ImGui::Begin("Register!", &show_register_window);
            ImGui::Text("Register:");
            //std::cout << "Register button clicked"<<std::endl;
            ImGui::Spacing();
            static char register_username[64] = "";
            ImGui::InputText("(Username)", register_username, 64);
            ImGui::Spacing();
            static char register_password1[64] = "";
            ImGui::InputTextWithHint("(Password)", "", register_password1, IM_ARRAYSIZE(register_password1), ImGuiInputTextFlags_Password);
            ImGui::Spacing();
            static char register_password2[64] = "";
            ImGui::InputTextWithHint("(Repeat Your Password)", "", register_password2, IM_ARRAYSIZE(register_password2), ImGuiInputTextFlags_Password);
            ImGui::Spacing();
            
            if (ImGui::Button("ENTER")) {
                if(strcmp(register_password1, register_password2))
                    ImGui::OpenPopup("Passwords are not matching!");
                else if (UserAlreadyExists(register_username, register_password1))
                    ImGui::OpenPopup("The user with such a name already exists!");
                else    ImGui::OpenPopup("You have successfully registered!");
                }


                ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                
                if (ImGui::BeginPopupModal("Passwords are not matching!", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    std::string mismatch_com = "Both the (Password) and the (Repeat Your Password) fields have to consist of the same characters.";
                    std::string mismatch_response = "I apologize.";
                    if (passwords_mismatch_again)
                    {
                        mismatch_com = "Passwords not matching again? Seriously???";
                        mismatch_response = "Ok, ok, I'll fix it. Now for real.";
                    }
                    //else
                    //{
                        //pass
                    //}
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), mismatch_com.c_str());
                    ImGui::Separator();
                    if (ImGui::Button(mismatch_response.c_str()))
                    { passwords_mismatch_again = true;  ImGui::CloseCurrentPopup(); }

                    ImGui::EndPopup();
                }

                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

                if (ImGui::BeginPopupModal("The user with such a name already exists!", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "You have to come up with something else :(");
                    ImGui::Separator();
                    if (ImGui::Button("Okey dokey"))
                    {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

                if (ImGui::BeginPopupModal("You have successfully registered!", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Congratulations, you have succeeded to make a valid account.");
                    ImGui::Separator();
                    if (ImGui::Button("OK"))
                    {
                        AddUser(register_username, register_password1);
                        ImGui::CloseCurrentPopup();
                        show_register_window = false;
                    }
                    ImGui::EndPopup();
                }
            
            ImGui::End();
        }

        //the journal window
        if (show_journal_window)
        {
            ImGui::SetNextWindowSize(ImVec2(800, 650));
            ImGui::Begin("YOUR OWN PERSONAL QUEST JOURNAL!", &show_journal_window);


            ImGui::Text("QUESTS:");
            ImGui::Checkbox("Main", &check_main);       ImGui::SameLine(); ImGui::Checkbox("Side", &check_side);   ImGui::SameLine();
            ImGui::Checkbox("Complete", &check_complete);   ImGui::SameLine(); ImGui::Checkbox("Failed", &check_failed);
            ImGui::Spacing();
            /*
            for(int i = 0; i < 3; i++)
                ImGui::Separator();
            */
            ImGui::Spacing();

            ImGui::BeginChild("ChildL", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f * 0.75f, ImGui::GetContentRegionAvail().y), true, ImGuiWindowFlags_None);
            if (check_main)
                ShowQuestlist("Main quests", login_username, QuestNames, active_quest);
            if (check_side)
                ShowQuestlist("Side quests", login_username, QuestNames, active_quest);
            if (check_complete)
                ShowQuestlist("Complete quests", login_username, QuestNames, active_quest);
            if (check_failed)
                ShowQuestlist("Failed quests", login_username, QuestNames, active_quest);
            ImGui::EndChild();

            
            ImGui::SameLine();

            
            ImGui::BeginChild("ChildR", ImVec2(0, 550), true, ImGuiWindowFlags_HorizontalScrollbar);
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
                ImGui::BeginChild("ChildR1", ImVec2(0, 400), true, ImGuiWindowFlags_HorizontalScrollbar);
                DisplayContents(active_quest);

                ImGui::EndChild();
                ImGui::PopStyleVar();

                if (ImGui::Button("REFRESH"))
                {
                    RefreshCommand();
                    QuestNames = UpdateQuests();
                    //for (int i = 0; i < QuestNames.size(); i++)
                    //    std::cout << QuestNames[i] << std::endl;
                    
                }
                ImGui::SameLine();
                if (ImGui::Button("ADD NEW QUEST"))
                {
                    show_add_quest_window = true;
                    show_journal_window = false;
                }
                ImGui::SameLine();
                if (ImGui::Button("MARK AS COMPLETE"))
                {
                    MarkActiveAs(active_quest, "Complete");
                }
                ImGui::SameLine();
                if (ImGui::Button("MARK AS FAILED"))
                {
                    MarkActiveAs(active_quest, "Failed");
                }

            }
            ImGui::EndChild();
            ImGui::End();
        }

        // add new quest window
        if (show_add_quest_window)
        {
            ImGui::SetNextWindowSize(ImVec2(600, 500));
            ImGui::Begin("ADD A NEW QUEST FOR YOUR JOURNAL!", &show_add_quest_window);

            ImGui::Text("Input the name of the quest:");
            ImGui::Spacing();
            ImGui::InputText("(Quest name without spaces)", new_quest_name, 128);
            ImGui::Spacing();

            ImGui::Text("Choose the type of the quest:");
            ImGui::Spacing();
            ImGui::ListBox("(Quest type)", &item_current_idx, items, IM_ARRAYSIZE(items), 2);
            ImGui::Spacing();

            ImGui::Text("Input the contents of the quest:");
            ImGui::Spacing();
            ImGui::InputTextMultiline("##source", new_quest_contents, IM_ARRAYSIZE(new_quest_contents), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 12), ImGuiInputTextFlags_AllowTabInput);
            ImGui::Spacing();

            if (ImGui::Button("ADD THE QUEST TO YOUR JOURNAL"))
            {
                AddNewQuest(login_username, new_quest_name, item_current_idx, new_quest_contents);  //idx = 0 <-> Main quest, idx = 1 <-> Side quest
                show_journal_window = true;
                show_add_quest_window = false;
            }

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Custom classes

class Quest
{
protected:
    std::string owner;
    std::string type;
    std::string name;
    std::string contents;

public:
    Quest(std::string owner, std::string type, std::string name, std::string contents)
    {
        this->owner = owner;
        this->type = type;
        this->name = name;
        this->contents = contents;
    }

    std::string getOwner()
    {
        return owner;
    }

    std::string getType()
    {
        return type;
    }

    std::string getName()
    {
        return name;
    }

    std::string getContents()
    {
        return contents;
    }
};

// Custom functions

void AddNewQuest(char new_quest_owner[], char new_quest_name[], int item_current_idx, char new_quest_contents[])
{
    //no support for spaces version
    
    std::string path("..\\Quests\\");
    std::string command("echo. ");
    std::string pipe(" >> ");
    std::string q_name(new_quest_name);
    std::string txt(".txt");
    command.append(pipe);
    command.append(path);
    command.append(new_quest_name);
    command.append(txt);
    

    //support for spaces version - [WIP]
    /*
    std::string path("..\\Quests\\");
    std::string command("echo. ");
    std::string pipe(" >> ");
    std::string q_name(new_quest_name);
    std::string txt(".txt");
    command.append(pipe);
    command.append("\"");
    command.append(path);
    command.append(new_quest_name);
    command.append(txt);
    command.append("\"");
    */

    const char* final_command = command.c_str();
    std::cout << std::endl << command;
    system(final_command);
    std::cout << std::endl << command;

    //do teraz dziala

    std::fstream new_quest_file;
    std::string path2("..\\Quests\\");
    path2.append(q_name);
    path2.append(".txt");
    new_quest_file.open(path2);
    std::string line;
    std::cout << std::endl << path2;
    
    if (new_quest_file.is_open())
    {
        new_quest_file << "Owner:\n";
        std::string owner_string(new_quest_owner);
        new_quest_file << owner_string;
        new_quest_file << "\n";

        new_quest_file << "Type:\n";
        if (item_current_idx == 0)
        {
            new_quest_file << "Main Quests\n";
        }
        else if (item_current_idx == 1)
        {
            new_quest_file << "Side Quests\n";
        }

        new_quest_file << "Name:\n";
        std::string name_string(new_quest_name);
        new_quest_file << name_string;
        new_quest_file << "\n";

        new_quest_file << "Contents:\n";
        std::string contents_string(new_quest_contents);
        new_quest_file << contents_string;
        new_quest_file << "\n";
    }
    

    new_quest_file.close();
}

void DisplayContents(std::string active_quest)
{
    //no spaces supported version
    /*
    std::fstream quest_file;
    std::string path("..\\Quests\\");
    path.append(active_quest.c_str());
    quest_file.open(path);
    std::string line;
    bool flag = false;
    */

    //spaces supported version
    std::fstream quest_file;
    std::string path("..\\Quests\\");
    path.append(active_quest.c_str());
    quest_file.open(path);
    std::string line;
    bool flag = false;

    if (quest_file.is_open())
    {
        while (getline(quest_file, line))
        {
            if (line == "Contents:")
            {
                flag = true;
            }
            if (flag == true)
            {
                ImGui::Text(line.c_str());
            }
        }
    }
    quest_file.close();
}

void MarkActiveAs(std::string active_quest, std::string category)   //"Complete" || "Failed"
{
    std::ifstream old_file;
    std::string path1("..\\Quests\\");
    path1.append(active_quest.c_str());
    old_file.open(path1);

    std::ofstream new_file;
    std::string path2("..\\Quests\\");
    path2.append("TemporaryFile.txt");
    new_file.open(path2);

    if (!old_file || !new_file)
    {
        return;
    }


    std::string TemporaryString;

    if (old_file.is_open() && new_file.is_open())
    {
        std::string line;
        std::string tmp;

        while (getline(old_file, line))
        {
            if (line == "Main Quests" || line == "Side Quests")
            {
                new_file << category;
                new_file << " Quests";
                new_file << "\n";
            }
            else
            {
                new_file << line << std::endl;
            }
        }
    }

    old_file.close();
    new_file.close();

    std::remove(path1.c_str());

    if (rename(path2.c_str(), path1.c_str()))
    {
        std::cout << std::endl << "Error renaming a file";
    }
    else
    {
        std::cout << std::endl << "File renamed successfully";
    }
}

bool IsOwnerOK(char username[], std::string file_name)
{
    std::fstream quest_file;
    std::string path("..\\Quests\\");
    path.append(file_name.c_str());
    quest_file.open(path);
    if (quest_file.is_open())
    {
        std::string word;
        while (quest_file.good())
        {
            quest_file >> word;
            if (word == "Owner:")
            {
                quest_file >> word;
                //std::cout << std::endl << word;
                break;
            }
        }
        if (!strcmp(word.c_str(), username))
        {
            //std::cout << std::endl << !strcmp(word.c_str(), username);
            return true;
        }
        else
        {
            //std::cout << std::endl << "false";
            return false;
        }
    }

    
}

std::string GetCategoryFromFile(std::string name)
{
    std::fstream quest_file;
    std::string path("..\\Quests\\");
    path.append(name.c_str());
    quest_file.open(path);
    if (quest_file.is_open())
    {
        std::string word;
        while (quest_file.good())
        {
            quest_file >> word;
            if (word == "Type:")
            {
                quest_file >> word;
                break;
            }
        }
        quest_file.close();
        word.append(" quests");
        //std::cout << std::endl << word;
        return word;
    }
    else
    {
        return " ";
    }
}

void RefreshCommand()
{
    std::string path("\"..\\Quests\"");
    std::string command("dir /a-d ");
    std::string pipe(" >> cache.txt");
    command.append(path);
    command.append(pipe);
    const char* final_command = command.c_str();
    std::remove("cache.txt");

    system(final_command);
}

std::vector <std::string> UpdateQuests()
{
    std::vector <std::string> QuestNames;
    std::fstream cache_file;
    cache_file.open("cache.txt");
    if (cache_file.is_open())
    {
        //char word[200];
        std::string word;

        while (cache_file.good())
        {
            cache_file >> word;
            
            for (int i = 0; i < end(word) - begin(word) - 3; i++)   //puts txt file names in QuestNames vector 
            {
                if (word[i] == '.')
                {
                    if (word[i + 1] == 't')
                    {
                        if (word[i + 2] == 'x')
                        {
                            if (word[i + 3] == 't')
                            {
                                QuestNames.push_back(word);
                            }
                        }
                    }
                }
            }
        }
    }
    cache_file.close();
    return QuestNames;
}

void ShowQuestlist(std::string category, char username[], std::vector <std::string>& QuestNames, std::string& activeQuest)
{
    //name of the list
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
    char res[100];
    strcpy(res, "Name of ");
    strcat(res, category.c_str());
    ImGui::BeginChild(res, ImVec2(ImGui::GetContentRegionAvail().x * 1.0f, 30), true, window_flags);
    ImGui::Text(category.c_str());
    ImGui::EndChild();

    //contents of the list(individual quests)
    ImGui::BeginChild(category.c_str(), ImVec2(ImGui::GetContentRegionAvail().x * 1.0f, 260), true, window_flags);
    ImGui::Spacing();
    if (ImGui::BeginTable("split", 1, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings))
    {
        

        for (int i = 0; i < QuestNames.size(); i++)
        {
            if (category == GetCategoryFromFile(QuestNames[i]) && IsOwnerOK(username, QuestNames[i]))
            {
                ImGui::TableNextColumn();
                if (ImGui::Button(QuestNames[i].c_str(), ImVec2(-FLT_MIN, 0.0f)))
                {
                    activeQuest = QuestNames[i];
                    std::cout << std::endl << activeQuest;
                }
            }

        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
}

bool ArePasswordsSame(char p1[], char p2[]) //obsolete
{
    if (sizeof(p1) != sizeof(p2))
        return false;
    for (int i = 0; i < sizeof(p1) / sizeof(p1[0])/8; i++)
    {
        if (p1[i] != p2[i])
            return false;
    }
    return true;
}

bool UserAlreadyExists(char username[], char password[])
{
    std::fstream users_file;
    users_file.open("users.txt");
    if (users_file.is_open())
    {
        std::string line;
        while (getline(users_file, line))
        {
            std::cout << line << std::endl;
            for (int i = 0; i < sizeof(username) / sizeof(username[0])/8; i++)
                std::cout << username[i];
            std::cout << std::endl;
            if (!strcmp(username, line.c_str())) { users_file.close(); return true; }
        }
    }
    users_file.close();
    return false;
}

void AddUser(char username[], char password[])
{
    std::fstream users_file;
    users_file.open("users.txt", std::ios::app);
    for (int i = 0; i < sizeof(username) / sizeof(username[0])/8; i++)
        users_file << username;
    users_file << std::endl;
    users_file.close();

    std::fstream passwords_file;
    passwords_file.open("passwords.txt", std::ios::app);
    for (int i = 0; i < sizeof(password) / sizeof(password[0])/8; i++)
        passwords_file << password;
    passwords_file << std::endl;
    passwords_file.close();

    //making the directory for the user's quests
    char res[100];
    strcpy(res, "mkdir ..\\Quests\\");
    std::system(strcat(res, username));
}

bool IsUserInDatabase(char username[], char password[]) //tbd
{
    std::fstream users_file;
    std::fstream passwords_file;
    users_file.open("users.txt");
    passwords_file.open("passwords.txt");
    if (users_file.is_open() && passwords_file.is_open())
    {
        std::string lineU;
        std::string lineP;
        while (getline(users_file, lineU))
        {
            getline(passwords_file, lineP);
            if ((strcmp(lineU.c_str(), username) == 0) && (strcmp(lineP.c_str(), password) == 0))
            {
                users_file.close();
                passwords_file.close();
                return true;
            }
        }
    }
    users_file.close();
    passwords_file.close();
    return false;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_WARP, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
