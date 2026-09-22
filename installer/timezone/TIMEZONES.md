# Installer Time Zones

Time zones are represented by IANA identifiers. The installer must use the target operating system's current timezone database rather than inventing daylight-saving rules.

The initial UI catalog includes UTC and representative North American, European, Asian, and Australian zones. The production image should ship the complete target OS timezone database.