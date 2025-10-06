#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sqlite3.h>
#include <time.h>

#define SERIAL_PORT "/dev/ttyACM0"
#define DB_NAME "distance.db"

int main(){
    int fd; // file descriptor
    struct termios tty;
    char buf[256];
    sqlite3 *db;
    char *err_msg = 0;

    //Open serial port
    fd = open(SERIAL_PORT, O_RDONLY | O_NOCTTY);
    if(fd < 0) { perror("Serial open"); return 1;}
    
    // Configure port for serial protocol
    tcgetattr(fd, &tty); // Get current serial port settings
    cfsetospeed(&tty, B9600); // Set output baud rate
    cfsetispeed(&tty, B9600); // Set input baud rate
    tty.c_cflag |= (CLOCAL | CREAD); // Ignore modem control lines and enable receiver so we can incoming data
    tty.c_cflag &= ~CSIZE; // Clear character size bits
    tty.c_cflag |= CS8; // Set 8 data bits per frame
    tty.c_cflag &= ~PARENB; // disable parity;
    tty.c_cflag &= ~CSTOPB; // Use 1 stop bit
    tcsetattr(fd, TCSANOW, &tty); // Apply all changes now without waiting for the port to close

    // Open SQlite DB
    if(sqlite3_open(DB_NAME, &db)) {
        fprintf(stderr, "Can't open DB : %s\n", sqlite3_errmsg(db));
        return 1;
    }
    
    // Create table if not exists
    const char *sql = "CREATE TABLE IF NOT EXISTS distance_log("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
    "distance REAL);";

    // This is a convenience function that executes an entire SQL statement (or multiple statements) in one call
    if(sqlite3_exec(db, sql, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr,"SQL error: %s\n", err_msg);
        sqlite3_free(err_msg); 
    }

    printf("Logging data... Press Ctrl+C to stop. \n");


char buf[64];          // temporary serial buffer
char line[128];        // assemble complete line
int pos = 0;           // current position in line[]

while (1) {
    int n = read(fd, buf, sizeof(buf)-1);
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            char c = buf[i];

            if (c == '\n') {
                // End of line
                line[pos] = '\0';   // terminate string
                printf("RAW: [%s]\n", line);

                // ---- Parsing ----
                if (strncmp(line, "Distance:", 9) == 0) {
                    float distance = atof(line + 9);

                    // build SQL query
                    char query[128];
                    snprintf(query, sizeof(query),
                             "INSERT INTO distance_log(distance) VALUES(%.2f);",
                             distance);

                    // execute insert
                    if (sqlite3_exec(db, query, 0, 0, &err_msg) != SQLITE_OK) {
                        fprintf(stderr, "SQL insert error: %s\n", err_msg);
                        sqlite3_free(err_msg);
                    } else {
                        printf("Logged: %.2f cm\n", distance);
                    }
                }

                // reset line buffer for next message
                pos = 0;

            } else if (c != '\r') {
                // store character if not CR
                if (pos < (int)sizeof(line) - 1) {
                    line[pos++] = c;
                }
            }
        }
    }

    usleep(200000); // 0.2s pause
}