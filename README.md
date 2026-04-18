🌾 GSM-Based Irrigation Pump Controller (ESP32 + SIM800L)
📌 Overview

This system allows remote control of an irrigation pump using:

📩 SMS commands
📞 Missed call (toggle control)

It uses:

ESP32 (controller)
SIM800L (GSM communication)
DOL Starter (motor control via relay pulses)
⚙️ Features
✔ Remote ON/OFF via SMS
✔ Missed call control (toggle motor)
✔ Admin-controlled access system
✔ Dynamic number registration (via SMS)
✔ EEPROM-based persistent storage
✔ Protection against unauthorized users
✔ Automatic SIM800L recovery
🔐 Access Control
👤 Admin Number
+919443275150

Only this number can:

Add/remove users
View registered numbers
📩 SMS Commands
🔧 Admin Commands
➕ Add New Number
ADD,+91XXXXXXXXXX

Description:
Adds a new user number to the allowed list.

❌ Delete Number
DEL,+91XXXXXXXXXX

Description:
Removes a number from the allowed list.

📋 List Numbers
LIST

Description:
Returns all registered numbers stored in EEPROM.

🚜 User Commands
▶️ Start Motor
On

Description:
Starts the motor using DOL starter (momentary relay pulse).

⏹ Stop Motor
Off

Description:
Stops the motor using DOL starter (momentary relay pulse).

📞 Call Control
📲 Missed Call
Call device number

Description:

If motor is OFF → Starts motor
If motor is ON → Stops motor
Call is automatically rejected (no cost)
💾 EEPROM Storage
Stores up to 10 user numbers
Data is retained after power restart
No need to reconfigure after reboot
🔄 System Workflow
Incoming SMS/Call
        ↓
Extract Number
        ↓
Check Authorization
        ↓
Execute Command
        ↓
Send Confirmation SMS
⚡ Hardware Behavior
DOL Starter Logic
Relay gives 1.5 sec pulse
Simulates physical START/STOP button
Prevents continuous relay activation
Relay Type
Active LOW
LOW  → Relay ON
HIGH → Relay OFF
📶 Network Notes (BSNL)
GSM 2G supported
SMS delay: 5–30 sec
Call control is faster & more reliable
⚠️ Important Notes
Always use +91 format for numbers
Only admin can modify users
Unknown numbers are ignored
Ensure stable 4V, 2A power for SIM800L
🧪 Example Usage
ADD,+917810010503
→ Added

On
→ Motor Started

Off
→ Motor Stopped

LIST
→ Shows all numbers
🚀 Future Enhancements (Optional)
💧 Dry-run protection (no water detection)
⏱️ Auto motor cutoff timer
📊 STATUS command (check motor state)
📶 Signal strength monitoring

If you want, I can convert this into:

📄 PDF documentation
🖼️ Block diagram included version
🏗️ GitHub-ready README.md (with badges, formatting)

Just tell 👍
