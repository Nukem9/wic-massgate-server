#pragma once

void HTTPService_Startup();
void HTTPService_Shutdown();
int HTTPService_HandleRequest(mg_connection* conn);
int HTTPService_HandleLog(const mg_connection *, const char *message);
int HTTPService_HandleGettext(mg_connection *conn, const mg_request_info *request_info);
int HTTPService_HandleButton(mg_connection *conn, const mg_request_info *request_info);

bool HTTPService_UsesUnicode(const mg_request_info *request_info);
void HTTPService_WriteUnicode(mg_connection *conn, char *string);
void HTTPService_WriteMultibyte(mg_connection *conn, char *string);