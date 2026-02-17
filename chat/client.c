#include "server.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

static void print_timestamp(WINDOW *w) {
  time_t now = time(NULL);
  struct tm *tm = localtime(&now);
  char ts[9];
  strftime(ts, sizeof ts, "[%H:%M] ", tm);
  wattron(w, A_DIM);
  wprintw(w, "%s", ts);
  wattroff(w, A_DIM);
}

static void redraw_users(WINDOW *users, WINDOW *users_content,
                         char names[][MAX_NAME_LEN], int ncount) {
  werase(users_content);
  int anon_count = 0;
  int row = 0;
  int max_rows = getmaxy(users_content);
  int max_cols = getmaxx(users_content);
  int matt_present = 0;

  for (int i = 0; i < ncount; i++) {
    if (strcmp(names[i], "@Matt") == 0)
      matt_present = 1;
  }

  if (!matt_present && row < max_rows) {
    wattron(users_content, COLOR_PAIR(3) | A_DIM);
    mvwaddnstr(users_content, row++, 0, "@Matt", max_cols);
    wattroff(users_content, COLOR_PAIR(3) | A_DIM);
  }

  for (int i = 0; i < ncount; i++) {
    if (strcmp(names[i], "Anonymous") == 0) {
      anon_count++;
      continue;
    }
    if (row >= max_rows)
      break;
    if (strcmp(names[i], "@Matt") == 0) {
      wattron(users_content, COLOR_PAIR(3) | A_BOLD);
      mvwaddnstr(users_content, row++, 0, names[i], max_cols);
      wattroff(users_content, COLOR_PAIR(3) | A_BOLD);
    } else {
      wattron(users_content, COLOR_PAIR(2));
      mvwaddnstr(users_content, row++, 0, names[i], max_cols);
      wattroff(users_content, COLOR_PAIR(2));
    }
  }

  if (anon_count > 0 && row < max_rows) {
    wattron(users_content, A_DIM);
    if (anon_count == 1)
      mvwaddnstr(users_content, row, 0, "Anonymous", max_cols);
    else {
      char label[MAX_NAME_LEN + 8];
      snprintf(label, sizeof label, "Anonymous (%d)", anon_count);
      mvwaddnstr(users_content, row, 0, label, max_cols);
    }
    wattroff(users_content, A_DIM);
  }

  wattron(users, COLOR_PAIR(4));
  box(users, 0, 0);
  mvwprintw(users, 0, 2, " users ");
  wattroff(users, COLOR_PAIR(4));
  wrefresh(users);
  wrefresh(users_content);
}

static void draw_input_border(WINDOW *w) {
  wattron(w, COLOR_PAIR(4));
  box(w, 0, 0);
  mvwprintw(w, 0, 2, " message ");
  wattroff(w, COLOR_PAIR(4));
}

int main(void) {
  int sockfd;
  struct sockaddr_un un_addr;

  memset(&un_addr, 0, sizeof un_addr);
  un_addr.sun_family = AF_UNIX;
  strncpy(un_addr.sun_path, SOCK_PATH, sizeof(un_addr.sun_path) - 1);

  sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sockfd == -1) {
    perror("client: socket");
    exit(EXIT_FAILURE);
  }

  if (connect(sockfd, (struct sockaddr *)&un_addr, sizeof un_addr) != 0) {
    perror("client: connect");
    exit(EXIT_FAILURE);
  }

  initscr();
  cbreak();
  noecho();
  start_color();
  use_default_colors();
  init_pair(1, COLOR_CYAN, -1);
  init_pair(2, COLOR_GREEN, -1);
  init_pair(3, COLOR_MAGENTA, -1);
  init_pair(4, COLOR_YELLOW, -1);
  init_pair(5, COLOR_RED, -1);

  int chat_height = LINES - 3;
  int chat_width = COLS - USERS_PANEL_WIDTH;
  int users_width = USERS_PANEL_WIDTH;

  int input_height = 3;
  int input_width = COLS;

  WINDOW *chat = newwin(chat_height, chat_width, 0, 0);
  WINDOW *input = newwin(input_height, input_width, LINES - 3, 0);
  WINDOW *users = newwin(chat_height, users_width, 0, chat_width);
  scrollok(chat, FALSE);
  scrollok(input, FALSE);
  scrollok(users, FALSE);

  WINDOW *chat_content = derwin(chat, chat_height - 2, chat_width - 3, 1, 2);
  scrollok(chat_content, TRUE);

  WINDOW *users_content = derwin(users, chat_height - 2, users_width - 3, 1, 1);
  scrollok(users_content, FALSE);

  wattron(chat, COLOR_PAIR(4));
  box(chat, 0, 0);
  mvwprintw(chat, 0, 2, " m-chat ");
  wattroff(chat, COLOR_PAIR(4));
  draw_input_border(input);
  wattron(users, COLOR_PAIR(4));
  box(users, 0, 0);
  mvwprintw(users, 0, 2, " users ");
  wattroff(users, COLOR_PAIR(4));

  keypad(input, TRUE);
  wmove(input, 1, 2);

  wrefresh(chat);
  wrefresh(users);
  wrefresh(input);

  char input_buf[MAX_MSG_LEN];
  memset(input_buf, 0, MAX_MSG_LEN);

  int len = 0;
  int cursor = 0;
  int input_offset = 0;

  int prefix_len = strlen(OPT_MSG_USERLIST);
  int pm_prefix_len = strlen(OPT_MSG_PM);
  int pmto_prefix_len = strlen(OPT_MSG_PMTO);
  int vis_width = COLS - 4;

  while (1) {
    struct pollfd fds[2];
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[1].fd = sockfd;
    fds[1].events = POLLIN;

    int ready = poll(fds, 2, -1);
    if (ready > 0) {
      if (fds[0].revents & POLLIN) {
        int ch = wgetch(input);

        if (ch == '\n') {
          input_buf[len++] = '\n';
          input_buf[len] = '\0';

          wclear(input);
          draw_input_border(input);
          wmove(input, 1, 2);
          wrefresh(input);

          if (len >= MAX_MSG_LEN) {
            wprintw(input, "\nMax message length exceeded");
            wrefresh(input);
          } else {
            ssize_t bytes_sent = send(sockfd, input_buf, len, 0);
            if (bytes_sent <= 0)
              perror("client: send");

            memset(input_buf, 0, MAX_MSG_LEN);
            len = 0;
            cursor = 0;
            input_offset = 0;
          }
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
          if (cursor > 0) {
            memmove(&input_buf[cursor - 1], &input_buf[cursor], len - cursor);
            len--;
            cursor--;
            input_buf[len] = '\0';

            if (cursor < input_offset)
              input_offset = cursor;

            wmove(input, 1, 2);
            wclrtoeol(input);
            int show = (len - input_offset < vis_width)
                            ? len - input_offset
                            : vis_width;
            for (int i = 0; i < show; i++)
              waddch(input, input_buf[input_offset + i]);

            draw_input_border(input);
            wmove(input, 1, 2 + cursor - input_offset);
            wrefresh(input);
          }
        } else if (ch == KEY_DC) {
          if (cursor < len) {
            memmove(&input_buf[cursor], &input_buf[cursor + 1], len - cursor - 1);
            len--;
            input_buf[len] = '\0';

            wmove(input, 1, 2);
            wclrtoeol(input);
            int show = (len - input_offset < vis_width)
                            ? len - input_offset
                            : vis_width;
            for (int i = 0; i < show; i++)
              waddch(input, input_buf[input_offset + i]);

            draw_input_border(input);
            wmove(input, 1, 2 + cursor - input_offset);
            wrefresh(input);
          }
        } else if (ch == KEY_LEFT) {
          if (cursor > 0) {
            cursor--;
            if (cursor < input_offset)
              input_offset = cursor;
            wmove(input, 1, 2 + cursor - input_offset);
            wrefresh(input);
          }
        } else if (ch == KEY_RIGHT) {
          if (cursor < len) {
            cursor++;
            if (cursor - input_offset >= vis_width)
              input_offset = cursor - vis_width + 1;
            wmove(input, 1, 2 + cursor - input_offset);
            wrefresh(input);
          }
        } else if (ch == KEY_HOME) {
          cursor = 0;
          input_offset = 0;
          wmove(input, 1, 2);
          wclrtoeol(input);
          int show = (len < vis_width) ? len : vis_width;
          for (int i = 0; i < show; i++)
            waddch(input, input_buf[i]);
          draw_input_border(input);
          wmove(input, 1, 2);
          wrefresh(input);
        } else if (ch == KEY_END) {
          cursor = len;
          if (cursor - input_offset >= vis_width)
            input_offset = cursor - vis_width + 1;
          if (input_offset < 0)
            input_offset = 0;
          wmove(input, 1, 2);
          wclrtoeol(input);
          int show = (len - input_offset < vis_width)
                          ? len - input_offset
                          : vis_width;
          for (int i = 0; i < show; i++)
            waddch(input, input_buf[input_offset + i]);
          draw_input_border(input);
          wmove(input, 1, 2 + cursor - input_offset);
          wrefresh(input);
        } else if (ch >= 32 && ch < 127) {
          if (len < MAX_MSG_LEN - 2) {
            memmove(&input_buf[cursor + 1], &input_buf[cursor], len - cursor);
            input_buf[cursor] = ch;
            len++;
            cursor++;
            input_buf[len] = '\0';

            if (cursor - input_offset >= vis_width)
              input_offset = cursor - vis_width + 1;

            wmove(input, 1, 2);
            wclrtoeol(input);
            int show = (len - input_offset < vis_width)
                            ? len - input_offset
                            : vis_width;
            for (int i = 0; i < show; i++)
              waddch(input, input_buf[input_offset + i]);

            draw_input_border(input);
            wmove(input, 1, 2 + cursor - input_offset);
            wrefresh(input);
          }
        }
      }

      if (fds[1].revents & POLLIN) {
        char buf[MAX_SEND_LEN];
        ssize_t bytes_recv = recv(sockfd, buf, MAX_SEND_LEN - 1, 0);
        if (bytes_recv < 0) {
          perror("client: recv from server");
          continue;
        } else if (bytes_recv == 0) {
          break;
        }
        buf[bytes_recv] = '\0';

        char *pos = buf;
        while (*pos) {
          if (strncmp(pos, OPT_MSG_EXIT, strlen(OPT_MSG_EXIT)) == 0) {
            print_timestamp(chat_content);
            wattron(chat_content, COLOR_PAIR(1));
            wprintw(chat_content, OPT_MSG_EXIT);
            wattroff(chat_content, COLOR_PAIR(1));
            wrefresh(chat);
            goto done;
          } else if (strncmp(pos, OPT_MSG_USERLIST, prefix_len) == 0) {
            char *list_start = pos + prefix_len;
            char *nl = strchr(list_start, '\n');
            if (nl)
              *nl = '\0';

            char names[MAX_CLIENTS][MAX_NAME_LEN];
            int ncount = 0;
            char *tok = strtok(list_start, ",");
            while (tok && ncount < MAX_CLIENTS) {
              snprintf(names[ncount], MAX_NAME_LEN, "%s", tok);
              ncount++;
              tok = strtok(NULL, ",");
            }

            redraw_users(users, users_content, names, ncount);
            pos = nl ? nl + 1 : pos + strlen(pos);
          } else if (strncmp(pos, OPT_MSG_PMTO, pmto_prefix_len) == 0) {
            char *data = pos + pmto_prefix_len;
            char *nl = strchr(data, '\n');
            if (nl)
              *nl = '\0';
            char *colon = strchr(data, ':');
            if (colon) {
              *colon = '\0';
              print_timestamp(chat_content);
              wattron(chat_content, COLOR_PAIR(5));
              wprintw(chat_content, "[Private Message]");
              wattroff(chat_content, COLOR_PAIR(5));
              wprintw(chat_content, " to ");
              wattron(chat_content, COLOR_PAIR(5));
              wprintw(chat_content, "%s", data);
              wattroff(chat_content, COLOR_PAIR(5));
              wprintw(chat_content, ": %s\n", colon + 1);
            }
            pos = nl ? nl + 1 : data + strlen(data);
          } else if (strncmp(pos, OPT_MSG_PM, pm_prefix_len) == 0) {
            char *data = pos + pm_prefix_len;
            char *nl = strchr(data, '\n');
            if (nl)
              *nl = '\0';
            char *colon = strchr(data, ':');
            if (colon) {
              *colon = '\0';
              print_timestamp(chat_content);
              wattron(chat_content, COLOR_PAIR(5) | A_BOLD);
              wprintw(chat_content, "[Private Message]");
              wattroff(chat_content, COLOR_PAIR(5) | A_BOLD);
              wprintw(chat_content, " from ");
              wattron(chat_content, COLOR_PAIR(5) | A_BOLD);
              wprintw(chat_content, "%s", data);
              wattroff(chat_content, COLOR_PAIR(5) | A_BOLD);
              wprintw(chat_content, ": %s\n", colon + 1);
            }
            pos = nl ? nl + 1 : data + strlen(data);
          } else {
            char *ul = strchr(pos, '\x01');
            char *end = ul ? ul : pos + strlen(pos);
            if (end > pos) {
              size_t len = end - pos;
              char segment[MAX_SEND_LEN];
              memcpy(segment, pos, len);
              segment[len] = '\0';
              char *colon = strstr(segment, ": ");
              int is_chat_msg = 0;
              int nlen = 0;
              if (colon) {
                nlen = colon - segment;
                is_chat_msg = 1;
                for (int c = 0; c < nlen; c++) {
                  if (segment[c] == ' ' || segment[c] == '[') {
                    is_chat_msg = 0;
                    break;
                  }
                }
              }
              print_timestamp(chat_content);
              if (is_chat_msg) {
                int admin = (nlen == 5 && strncmp(segment, "@Matt", 5) == 0);
                wattron(chat_content, COLOR_PAIR(admin ? 3 : 2) | A_BOLD);
                wprintw(chat_content, "%.*s", nlen, segment);
                wattroff(chat_content, COLOR_PAIR(admin ? 3 : 2) | A_BOLD);
                wprintw(chat_content, "%s", segment + nlen);
              } else {
                wattron(chat_content, COLOR_PAIR(1));
                wprintw(chat_content, "%s", segment);
                wattroff(chat_content, COLOR_PAIR(1));
              }
            }
            pos = end;
          }
        }

        wrefresh(chat);
        wrefresh(chat_content);
        wrefresh(input);
      }
    }
  }

done:
  close(sockfd);
  endwin();
  puts("Connection terminated");
  return 0;
}
