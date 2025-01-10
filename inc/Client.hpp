#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>
# include <vector>
# include <algorithm>

class Client
{
  public:
    Client(int clientfd);
    ~Client();

    bool isRegistered() const;
    void setRegistered(bool registered);
    int getClientfd() const;
    void setUsername(const std::string &username);
    void setFirstName(const std::string &fname);
    std::string getFirstName() const;
    std::string getLastName() const;
    void setLastName(const std::string &lname);
    std::string getUsername() const;
    void setNickname(const std::string &nickname);
    std::string getNickname() const;
    bool isLoggedIn() const;
    void setLoggedIn(bool status);

    // Methods to manage client connection state
    void setNickReceived(bool received);
    void setUserReceived(bool received);
    bool getNickReceived() const;
    bool getUserReceived() const;
    void setOperator(bool value);
    bool getOperator() const;

    const std::string& getBuffer() const { return _buffer; }
    void appendToBuffer(const std::string& data) { _buffer.append(data); }
    void eraseFromBuffer(size_t pos, size_t len) { _buffer.erase(pos, len); }

  private:
    int _clientfd;
    std::string _username;
    std::string _fname;
    std::string _lname;
    std::string _nick;
    std::string _buffer;

    // Member variables to track connection state
    bool _registered;
    bool _nickReceived;
    bool _userReceived;
    bool _loginStatus;
    bool _operator;
};

// class Group
// {
//   public:
//     Group(const std::string &name);
//     ~Group();
//     std::string getGroupName() const;
//     void setGroupName(const std::string &name);
//     void addMember(Client *client);
//     void removeMember(Client *client);
//     void addOperator(Client *client);
//     void removeOperator(Client *client);
//     bool isOperator(Client *client) const;
//     void setOwner(Client *client);
//     Client* getOwner() const;
//     void setInviteOnly(bool inviteOnly);
//     bool getInviteOnly() const;
//     void setPassword(const std::string &password);
//     std::string getPassword() const;
//     void setPasswordOn(bool passwordOn);
//     bool getPasswordOn() const;
//     void setMemberLimit(int limit);
//     int getMemberLimit() const;
//     void setMemberLimitOn(bool memberLimitOn);
//     bool getMemberLimitOn() const;
//     void setTopic(const std::string &topic);
//     std::string getTopic() const;
//     void setTopicBool(bool topicBool);
//     bool getTopicBool() const;
//     void addCurrentlySignedIn(Client *client);
//     std::vector<Client *> getCurrentlySignedIn() const;
//     std::vector<Client *> getMembersList() const;
//     std::vector<Client *> getOperatorList() const;

//   private:
//     std::string groupName;
//     std::vector<Client *> membersList;
//     std::vector<Client *> operatorList;
//     std::vector<Client *> currentlySignedIn;
//     bool inviteOnly;
//     std::string password;
//     bool passwordOn;
//     int memberLimit;
//     bool memberLimitOn;
//     std::string topic;
//     bool topicBool;
//     Client* owner;
// };

#endif