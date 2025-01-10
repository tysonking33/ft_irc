#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include <set>
# include "Client.hpp"

class Channel
{
  public:
    Channel(const std::string& name);
    Channel(const std::string& name, const std::string& key);
    const std::string& getName() const;
    bool isUserInChannel(Client* client) const;
    void addUser(Client* client);
    void removeUser(Client* client);
    bool isInviteOnly() const;
    bool isInvited(Client* client) const;
    bool hasKey() const;
    const std::string& getKey() const;
    bool isFull() const;
    void setInviteOnly(bool inviteOnly);
    void setKey(const std::string& key);
    void setLimit(size_t limit);
    size_t getLimit() const;
    const std::set<Client*>& getUsers() const;
    void addOperator(Client* client);
    void removeOperator(Client* client);
    void setTopic(const std::string& topic);
    const std::string& getTopic() const;
    void setTopicRestriction(bool restriction);
    bool isTopicRestriction() const;

  private:
    std::string name;
    std::set<Client*> users;
    bool inviteOnly;
    std::set<Client*> invitedUsers;
    std::string key;
    size_t limit;
    std::string topic;
    bool topicRestriction;
};

#endif
