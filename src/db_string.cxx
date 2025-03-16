/*
Copyright (c) 2020-25 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/

// TODO: Migrate from GDBM to LMDB
// PREREQ: https://github.com/LMDB/lmdb.git

#include "db_string.hxx"


/*

#include <stdio.h>
#include <lmdb.h>

MDB_val key, data;
MDB_txn *txn;
MDB_dbi dbi;
MDB_env *env; // Assuming you have an initialized LMDB environment

// Begin a transaction
if (mdb_txn_begin(env, NULL, MDB_RDONLY, &txn) != 0) {
    fprintf(stderr, "Failed to begin transaction\n");
}

// Open the database (assuming the database has been opened previously)
if (mdb_dbi_open(txn, NULL, 0, &dbi) != 0) {
    fprintf(stderr, "Failed to open database\n");
}

// Set the key
key.mv_size = strlen(term);
key.mv_data = term;

// Fetch the value
if (mdb_get(txn, dbi, &key, &data) == 0) {
    // Successfully retrieved data
    printf("Value: %.*s\n", (int)data.mv_size, (char *)data.mv_data);
} else {
    fprintf(stderr, "Key not found\n");
}

// Commit and close the transaction
mdb_txn_commit(txn);


*/
