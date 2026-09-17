#include <stdio.h>

// Structure to hold historical data
typedef struct {
    int start_year;
    int end_year;
    char capital[30];
    char empire[50];
    char region[30];
} CapitalTimeline;

int main() {
    // Array of key historical capitals spanning 1161 to 1611
    CapitalTimeline history[] = {
        // Northern / Central India (Delhi Sultanate & Mughals)
        {1161, 1206, "Lahore / Ghazni", "Ghurid Empire", "North"},
        {1206, 1327, "Delhi", "Delhi Sultanate (Mamluk to Tughlaq)", "North"},
        {1327, 1334, "Daulatabad (Devagiri)", "Delhi Sultanate (Mughal-precursor era)", "Central"},
        {1334, 1506, "Delhi", "Delhi Sultanate (Tughlaq to Lodi)", "North"},
        {1506, 1530, "Agra", "Lodi Dynasty / Early Mughal Empire", "North"},
        {1530, 1540, "Delhi", "Mughal Empire (Humayun)", "North"},
        {1540, 1556, "Sasaram / Delhi", "Suri Empire", "North"},
        {1556, 1571, "Agra", "Mughal Empire (Akbar)", "North"},
        {1571, 1585, "Fatehpur Sikri", "Mughal Empire (Akbar)", "North"},
        {1585, 1598, "Lahore", "Mughal Empire (Akbar)", "North"},
        {1598, 1611, "Agra", "Mughal Empire (Akbar / Jahangir)", "North"},

        // Southern India (Vijayanagara Empire - major contemporary power)
        {1336, 1565, "Vijayanagara (Hampi)", "Vijayanagara Empire", "South"},
        {1565, 1611, "Penukonda / Chandragiri", "Aravidu Dynasty (Late Vijayanagara)", "South"}
    };

    int total_records = sizeof(history) / sizeof(history[0]);

    printf("=================================================================================\n");
    printf("              HISTORICAL CAPITALS OF INDIA TIMELINE (1161 - 1611)                \n");
    printf("=================================================================================\n");
    printf("%-12s %-30s %-25s %-10s\n", "Timeline", "Capital City", "Empire/Dynasty", "Region");
    printf("---------------------------------------------------------------------------------\n");

    for(int i = 0; i < total_records; i++) {
        printf("%d - %d   %-30s %-25s %-10s\n", 
               history[i].start_year, 
               history[i].end_year, 
               history[i].capital, 
               history[i].empire, 
               history[i].region);
    }
    
    printf("=================================================================================\n");
    return 0;
}

