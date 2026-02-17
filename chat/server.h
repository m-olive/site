#include <poll.h>

#define BACKLOG 5
#define SOCK_PATH "/tmp/chat.sock"

#define DELIMITERS " \n\t\f\v"
#define MAX_CLIENTS 50
#define MAX_NAME_LEN 18
#define MAX_SEND_LEN 5164
#define MAX_MSG_LEN 5120

#define USERS_PANEL_WIDTH 22

#define OPT_MSG_EXIT "Exiting m-chat\n"
#define OPT_MSG_HELP                                                           \
  "\n"                                                                         \
  "/nick <name>          -  change your nickname\n"                            \
  "/msg <user> <message> -  send a private message\n"                          \
  "/help                 -  show this message\n"                               \
  "/exit                 -  disconnect\n"
#define OPT_MSG_MENU "/nick to change your nickname\n/exit to exit\n"
#define OPT_MSG_NICK_SUCCESS "Your nickname has been changed to:"
#define OPT_MSG_USERLIST "\x01USERLIST:"
#define OPT_MSG_PM "\x01PM:"
#define OPT_MSG_PMTO "\x01PMTO:"

#define ERR_NICK_EMPTY "Nickname change failed: nickname field empty\n"
#define ERR_NICK_LEN                                                           \
  "Nickname change failed: nickname exceeds the max length [18 characters]\n"
#define ERR_OPT_NOT_FOUND "Menu option not found:"

typedef struct {
  int fd;
  char name[MAX_NAME_LEN];
} client_t;

typedef struct {
  struct pollfd *fds;
  client_t *clients;
  int count;
} client_list_t;
