#pragma once

#include "server_session.h"
// #include <cstdint>
// #include <memory>

bool checkBaseStructureSrv(PGconn *conn);

bool initDatabaseOnServer(PGconn *conn);

bool systemInitForTest(ServerSession &serverSession, PGconn *conn);
