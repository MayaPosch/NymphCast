#!/bin/sh

# Test file transfer:
# Checks that a file can be 

NSERVER="bin/test_server"
NCLIENT="../client/bin/nymphcast_client"


# Configure and start server.
./$NSERVER -t hash_only

# Configure and start client.
./$NCLIENT -f ../client/bin/amv_test.mpg
