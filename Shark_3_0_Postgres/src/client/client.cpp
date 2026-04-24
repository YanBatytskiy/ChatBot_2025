#include "core/chat_system/chat_system.h"
#include "client/client_session.h"
#include "client/menu/1_0_entrance_menu.h"
#include "client/menu/1_1_creation.h"
#include "core/system/system_function.h"
#include <clocale>
#include <iostream>
#include <unistd.h>

/**
 * @brief Main entry point for the chat system application.
 * @return 0 on successful execution or exit.
 * @details Initializes the chat system, handles user authentication
 * (registration/login), and manages the main program loop.
 */
int main() {

  std::setlocale(LC_ALL, "");
  enableUTF8Console();

  // const std::string envName = getEnvironmentName();
  // const std::string home    = getHomeDir();

  // if (home.empty()) {
  //     std::cerr << "Error: could not determine the home directory\n";
  //     return 1;
  // }

  // std::filesystem::path dbDir = getDbDirPath(home, envName);
  // std::string err;
  // if (!ensureDbDirExists(dbDir, err)) {
  //     std::cerr << "Error creating the directory: " << err << "\n";
  //     return 1;
  // }

  // std::filesystem::path dbFile = getDbFilePath(dbDir);
  // sqlite3* db = nullptr;
  // if (!openClientDb(dbFile, &db, err)) {
  //     std::cerr << "Error opening the database: " << err << "\n";
  //     return 1;
  // }

  // std::cout << "Database opened: " << dbFile << "\n";

  ChatSystem clientSystem;
  ClientSession clientSession(clientSystem);
  clientSystem.setIsServerStatus(false);

  // Find the server and create a connection
  if (clientSession.findServerAddress(clientSession.getserverConnectionConfigCl(),
                                      clientSession.getserverConnectionModeCl())) {

    clientSession.createConnection(clientSession.getserverConnectionConfigCl(),
                                   clientSession.getserverConnectionModeCl());
    // clientSession.reidentifyClientAfterConnection();
  } else
    clientSession.getserverConnectionModeCl() = ServerConnectionMode::Offline;

  short userChoice;

  // Main program loop
  while (true) {
    // Reset active user
    clientSession.setActiveUserCl(nullptr);

    // Display authentication menu and get user choice
    userChoice = entranceMenu();

    // Handle user choice for registration, login, or exit
    switch (userChoice) {
      case 0: // Exit the program
        return 0;
      case 1: // Register a new user
        userCreation(clientSession);
        break;
      case 2: // Log in an existing user
        if (userLoginInsystem(clientSession))
          loginMenuChoice(clientSession);
        break;
      default:
        break; // Handle invalid choices
    }
  }

  std::cout << std::endl;

  close(clientSession.getSocketFd());

  return 0;
}
