#include "../inc/Channel.hpp"

Channel::Channel(const std::string& name)
  : name(name), inviteOnly(false), limit(0)
{
}

Channel::Channel(const std::string& name, const std::string& key)
  : name(name), inviteOnly(false), key(key), limit(0)
{
}

const std::string& Channel::getName() const
{
  return name;
}

bool Channel::isUserInChannel(Client* client) const
{
  return users.find(client) != users.end();
}

void Channel::addUser(Client* client)
{
  users.insert(client);
}

void Channel::removeUser(Client* client)
{
  users.erase(client);
}

bool Channel::isInviteOnly() const
{
  return inviteOnly;
}

bool Channel::isInvited(Client* client) const
{
  return invitedUsers.find(client) != invitedUsers.end();
}

bool Channel::hasKey() const
{
  return !key.empty();
}

const std::string& Channel::getKey() const
{
  return key;
}

bool Channel::isFull() const
{
  return limit > 0 && users.size() >= limit;
}

void Channel::setInviteOnly(bool inviteOnly)
{
  this->inviteOnly = inviteOnly;
}

void Channel::setKey(const std::string& key)
{
  this->key = key;
}

void Channel::setLimit(size_t limit)
{
  this->limit = limit;
}

size_t Channel::getLimit() const
{
  return this->limit;
}

const std::set<Client*>& Channel::getUsers() const
{
  return users;
}

void Channel::addOperator(Client* client)
{
  client->setOperator(true);
}

void Channel::removeOperator(Client* client)
{
  client->setOperator(false);
}

void Channel::setTopic(const std::string& topic)
{
  this->topic = topic;
}

const std::string& Channel::getTopic() const
{
  return topic;
}

void Channel::setTopicRestriction(bool restriction)
{
  this->topicRestriction = restriction;
}

bool Channel::isTopicRestriction() const
{
  return topicRestriction;
}
