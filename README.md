# DCN_Chat_Program
[UIC Data Communication Workshop 24S Group Assignment2]

## Project Description
Our purpose is to create a chat room that allows local area users to freely communicate with each other one-on-one as well as group chat while ensuring individual privacy.

We also give administrators the ability to manage users and all chatting groups to ensure a friendly chatting environment.

And if users do not know how to operate, we will also have a prompt to guide them through.

## Project Structure
- **Server-Side:**
  1. **Accept** client connections & **Receive** messages.
  2. For each message, use regular expressions to **determine its message type & response** to sender.
- **Client-Side:**
  1. **Send & Receive** Messages.
  2. **Announce available Username**
  3. **Client-Side Hints:** #help & #quit.

## Features
1. **Public/private chatting:** Direct Message, Chatroom Message
2. **Group Chatting:** create, add by passcode, delete by admin, delete user by admin
3. **Server-side Commands:** delete client, shutdown Server
4. **Client-side Hints:** Help & Get online client names
5. **Retry Mechanism:** client username declaration, client connection
6. **Only display messages posted by clients**

## Usage
> Public / Private Chatting
  1. Public: `<msg>`
  2. Private: `@username <msg>`
>Group Chatting: Unordered List (Hash)
  1. Create: `Group @[<usernames>] Group name, password`
  2. Add: `Group_add @Group name, password`
  3. Chat: `@[Group name] <msg>`
  4. Group Admin Only:
     1. Delete: `Group_del @Group name, password`
     2. Del People: `Group_delp @Group name, username, password`
