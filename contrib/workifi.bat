:: Simple wrapper to keep CMD open after the program is finished,
:: it also sets things up for display of Arabic language characters.

@ECHO OFF
chcp 65001
.\workifi.exe
echo Exit Code is %errorlevel%
PAUSE
