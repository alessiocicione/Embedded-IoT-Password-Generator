# IoT/Embedded Password Generator

This is a personal project implements a standalone device capable of generating customizable passwords and delivering them via a local Wi-Fi web server.

The core goal is to provide a physical, offline mechanism for creating secure passwords for various accounts and services.

---

## Security and Features

### Functionality

* **Customizable Generation:** Passwords can be customized from **8 to 32 characters** and can include special symbols, numbers, and case-sensitive letters.
* **Profile Persistence:** Custom generation settings (length, character set) are saved to **EEPROM** memory, allowing users to recall specific profile settings without reconfiguration.
* **Deep Sleep Mode:** The device is built to utilize **Deep Sleep Mode** effectively, maximizing battery life and ensuring high autonomy.

### Delivery

* **Local Web Server Sharing:** Generated passwords are temporarily displayed on a local **Web Server**. Users can securely retrieve the password on any smartphone or device connected to the same local Wi-Fi network, avoiding reliance on external cloud services.
* **Local Display:** Passwords are also immediately shown on an **LCD display** for immediate, physical viewing.

---

## Architecture and Components

The system is designed around a microcontroller with integrated Wi-Fi capabilities (e.g., ESP32).

| Component | Role | Function |
| :--- | :--- | :--- |
| **Microcontroller** | Core Logic/Wi-Fi | Runs the generation algorithm, manages EEPROM, and hosts the web server. |
| **EEPROM** | Data Persistence | Stores customizable profile settings and configurations. |
| **LCD Display** | Physical Output | Displays the generated password and device status. |
| **Local Web Server**| Secure Output | Provides the local interface (`http://[device-IP]`) for smartphone access. |

---

## Usage

This section details the user workflow and the specific functions of the device's control buttons. For wiring details and component connections, refer to the **`hardware/`** circuit schematic.

### Device Startup and State

1.  **Power Up:** Upon startup, the device immediately displays the **local Web Server IP address** on the LCD screen and loads the last saved configuration profile from EEPROM.
2.  **Web Server Access:** Generated passwords can be securely retrieved via the displayed IP address using a smartphone or PC connected to the same Wi-Fi network.

### Control Functions (Refer to `hardware/` Schematics)

The device is controlled via five physical buttons with the following functions:

| Button Pin (See Schematics) | Name | Primary Function | Configuration Function |
| :--- | :--- | :--- | :--- |
| **BTN_LEN_UP** | **Length Up** | Increments password length (**8 to 32** characters). | Scrolls **Up** in the configuration menu. |
| **BTN_LEN_DOWN** | **Length Down** | Decrements password length (**8 to 32** characters). | Scrolls **Down** in the configuration menu. |
| **BTN_GENERATE** | **Generate/Toggle** | Generates the password and displays it on the LCD/Web Server. | Toggles (Activates/Deactivates) the selected setting. |
| **BTN_SLEEP** | **Sleep/Exit** | Puts the device into **Deep Sleep Mode**. | Exits the configuration menu (saves changes). |
| **BTN_WAKE** | **Wake** | Wakes the device from Deep Sleep Mode. | N/A |

### Workflow

* **Adjust Length:** Use **Button BTN_LEN_UP** (Length Up) and **Button BTN_LEN_DOWN** (Length Down) to set the desired password length (8-32 characters).
* **Generate Password:** Press **Button BTN_GENERATE** to generate the password.

### Configuration Menu

1.  **Enter Menu:** Press and hold **Button BTN_LEN_UP + Button BTN_LEN_DOWN** simultaneously.
2.  **Navigate:** Use **Button BTN_LEN_UP** and **Button BTN_GENERATE** to scroll through the menu options.
3.  **Modify:** Use **Button BTN_GENERATE** to activate or deactivate generation settings (e.g., inclusion of symbols, numbers).
4.  **Exit:** Press **Button BTN_SLEEP** to save changes and exit the configuration menu.
