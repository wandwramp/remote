/*
########################################################################
# This file is part of the down section of remote for WRAMP.
#
# Copyright (C) 2019 The University of Waikato, Hamilton, New Zealand.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
########################################################################
*/

#include <stdio.h>
#include <time.h>
#include <stdlib.h>

// Uncomment this line and rebuild to get a descriptive log
// in the working directory when running down.
//#define DEBUG

int main(int argc, char *argv[])
{
    FILE *file;
    #ifdef DEBUG
    FILE *log;
    #endif
    char line[132];
    int lines;
    int ch;          /*Yep it's an int.  See man getchar*/

    time_t t;
    srand((unsigned) time(&t));

    if ( argc < 2 )
    {
        fprintf(stderr, "Please specify a filename to upload.\n");
        return -1;
    }

    file = fopen(argv[1], "r");
    if ( file == NULL )
    {
        fprintf(stderr, "Could not open file: %s\n", argv[1]);
        return -1;
        //perror(argv[0]);
    }

    #ifdef DEBUG
    log = fopen("down.log", "w");
    if ( log == NULL )
    {
        fprintf(stderr, "Could not open file: %s\n", argv[1]);
        fclose(file);
        return -1;
    }
    #endif

    fprintf(stderr, "Uploading file: %s\n", argv[1]);

    lines = 0;
    while ( NULL != fgets(line, 132, file) )
    {
        printf("%s", line);
        ++lines;
        fflush(NULL);

        #ifdef DEBUG
        fprintf(log, "Sent line %d\n", lines);
        fprintf(log, "%s\n", line);
        #endif

        while (1)
        {
            ch = getchar();
            #ifdef DEBUG
            fprintf(log, "Received %c", ch);
            #endif

            // If monitor sends a confirmation '.',
            // tell the user and send the next line
            if ( ch == '.' )
            {
                #ifdef DEBUG
                fprintf(log, ", breaking out of while\n");
                #endif
                fprintf(stderr, "%c", ch);
                fflush(NULL);
                break;
            }
            // If monitor didn't receive the line properly,
            // tell the user and send the line again
            else if ( ch == 'F' || ch == 'S' || ch == 'C' || ch == '?' )
            {
                #ifdef DEBUG
                fprintf(stderr, "%c", ch);
                ch = getchar();
                fprintf(stderr, "%c", ch);
                fprintf(stderr, "%d", lines);

                fprintf(log, "%c", ch);
                #else
                ch = getchar();
                #endif

                // Wait a bit before resending so the serial port has time to
                // catch up and resync the clock
                // This amount of time allows a 300bps connection to resync
                struct timespec sleep;
                sleep.tv_sec = 0;
                sleep.tv_nsec = 7500000L;
                #ifdef DEBUG
                fprintf(log, ", sleeping for %ldns\n", sleep.tv_nsec);
                #endif
                nanosleep(&sleep, NULL);

                printf("%s", line);
                fflush(NULL);

                #ifdef DEBUG
                fprintf(log, "Resending line %d\n", lines);
                fprintf(log, "%s\n", line);
                #endif
            }
            // If monitor doesn't send a recognised response, wait for it to send
            // the cli prompt " > " and then quit unhappily
            else
            {
                #ifdef DEBUG
                fprintf(log, ", unrecognised character. Waiting for prompt.");
                #endif

                char old1 = '\0', old2 = '\0', old3;
                fflush(NULL);

                old3 = ch;

                while (!(old1 == ' ' && old2 == '>' && old3 == ' '))
                {
                    if (old1 != '\0')
                    {
                        fprintf(stderr, "%c", old1);
                    }

                    old1 = old2;
                    old2 = old3;
                    old3 = getchar();
                }
                printf("\n");

                //fprintf(stderr, "\nError sending file, received '%c'.\n", ch);
                #ifdef DEBUG
                fclose(log);
                #endif
                return -1;
            }
        }
    }
    fprintf(stderr, "\n%s\n", fgets(line, 132, stdin));
    fprintf(stderr, "%s", fgets(line, 132, stdin));
    printf("\n");

    #ifdef DEBUG
    fclose(log);
    #endif

    return 0;
}

