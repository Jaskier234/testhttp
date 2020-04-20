
/* Creates connection with remote host described in address_string.
   address_string = "ip:port".
   Returned: socekt_id -> Connection established correctly
                    -1 -> Some error occured
   If error occurs, program is terminated */
int make_connection(char *address_string);
