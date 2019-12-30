//
// Created by cherit on 12/29/19.
//
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/msg.h>
#include <signal.h>
#include "server_operations.h"
#include "group.h"
#include "../definitions.h"


int login_user(UserList* userList, LoginReq* request, key_t main_key) {
    ActionResponse* login_response = (ActionResponse*)malloc(sizeof(ActionResponse));
    login_response->mtype = request->pid;
    User* user = find_on_usr_list(userList, request->username);
    int success;
    if(user == NULL)
    {
        printf("Failed login request. User \"%s\" doesn't exists.\n", request->username);
        login_response->success = 0;
        strcpy(login_response->content, "0");
    }
    else if(strcmp(user->password, request->password) == 0)
    {
        if (user->client_pid != 0)
        {
            logout_user(user);
        }
        user->ipc_id = msgget(REQ_RES_SHIFT+request->pid, 0666|IPC_CREAT);
        user->client_pid = request->pid;

        login_response->success = 1;
        sprintf(login_response->content,"%d", user->ipc_id);
        printf("Login success. User \"%s\", welcome.\n", request->username);
    }
    else
    {
        login_response->success = 0;
        strcpy(login_response->content, "0");
        printf("Failed login request. Incorrect password from %d.\n", request->pid);
    }
    msgsnd(main_key, login_response, sizeof(ActionResponse)- sizeof(long), 0);
    success = login_response->success;
    free(login_response);
    return success;
}

int logout_user(User *user) {
    make_response(user->ipc_id, LOGOUT_REQ+REQ_RES_SHIFT, 1, "User successfully signed out\n");
    printf("User %s has been signed out\n", user->username);
    kill(user->client_pid, SIGINT);
    msgctl(user->ipc_id, IPC_RMID, 0);
    user->client_pid = 0;
    user->ipc_id = -1;
    return 1;
}

int list_active_users(UserList* userList, User* user){
    char content[MAX_CONTENT_SIZE] = "\t";
    UserList* currentUser = userList;
    while(currentUser != NULL)
    {
        if(currentUser->user->client_pid > 0)
        {
            strcat(content, "\t");
            strcat(content, currentUser->user->username);
            strcat(content, "\n");
        }
        currentUser = currentUser->next;
    }
    make_response(user->ipc_id, ACTIVE_USR_REQ+REQ_RES_SHIFT, 1, content);
    return 1;
}

int list_users_in_group(User* user, Group* group){
    char content[MAX_CONTENT_SIZE] = "\t";
    if(group!=NULL)
    {
        UserList* currentUser = group->users;
        while(currentUser != NULL && currentUser->user != NULL)
        {
            strcat(content, "\t");
            strcat(content, currentUser->user->username);
            strcat(content, "\n");
            currentUser=currentUser->next;
        }
        make_response(user->ipc_id, USR_IN_GRP_REQ+REQ_RES_SHIFT, 1, content);
        return 1;
    }
    else
    {
        make_response(user->ipc_id, USR_IN_GRP_REQ+REQ_RES_SHIFT, -1, "Group doesn't exists.\n");
        return -1;

    }
}

int list_groups(GroupList* groupList, User* user){
    char content[MAX_CONTENT_SIZE] = "\t";
    GroupList* currentGroup = groupList;
    while(currentGroup != NULL)
    {
        strcat(content, "\t");
        strcat(content, currentGroup->group->name);
        strcat(content, "\n");
        currentGroup = currentGroup->next;
    }
    make_response(user->ipc_id, LIST_GRP_REQ+REQ_RES_SHIFT, 1, content);
    return 1;
}

int sign_to_group(Group * group, User* user)
{
    char content[MAX_CONTENT_SIZE];
    int status;
    if(group != NULL)
    {
        if(add_user_to_grp(user, group)){
            sprintf(content, "Successfully added user %s to group %s.", user->username, group->name);
            status = 1;
        }
        else
        {
            sprintf(content, "User %s already exists in group %s.", user->username, group->name);
            status = 2;
        }
    }
    else
    {
        sprintf(content, "Group doesn't exists");
        status = 0;
    }
    printf("%s\n", content);
    make_response(user->ipc_id, GRP_SIGNIN_REQ+REQ_RES_SHIFT, status, content);
    return status;
}
int sign_out_group(Group * group, User* user)
{
    char content[MAX_CONTENT_SIZE];
    int status;
    if(group != NULL)
    {
        if(remove_user_from_grp(user, group)){
            sprintf(content, "Successfully removed user %s from group %s.", user->username, group->name);
            status = 1;
        }
        else
        {
            sprintf(content, "User %s don't exists in group %s.", user->username, group->name);
            status = 2;
        }
    }
    else
    {
        sprintf(content, "Group %s doesn't exists",  group->name);
        status = 0;
    }
    printf("%s\n", content);
    make_response(user->ipc_id, GRP_SIGNOUT_REQ+REQ_RES_SHIFT, status, content);
    return status;
}

int end_sessions(key_t main_key, UserList *userList) {
    UserList* currentEl = userList;
    while(currentEl != NULL)
    {
        if(currentEl->user->ipc_id >=0)
            logout_user(currentEl->user);
        currentEl = currentEl->next;
    }
    msgctl(main_key, IPC_RMID, 0);
    return 0;
}
