#include "RedisManager.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>

using nlohmann::json;
using std::cerr;
using std::cout;
using std::endl;
using std::nullopt;
using std::stringstream;
using std::to_string;

RedisManager::RedisManager(const string &host, int port)
    : context(nullptr), host(host), port(port), connected(false),
      cache_hits(0), cache_misses(0)
{
}

RedisManager::~RedisManager()
{
    if (context)
    {
        redisFree(context);
        context = nullptr;
    }
}

bool RedisManager::connect()
{
    if (connected)
    {
        return true;
    }

    struct timeval timeout = {1, 500000}; // 1.5 seconds
    context = redisConnectWithTimeout(host.c_str(), port, timeout);

    if (context == nullptr || context->err)
    {
        if (context)
        {
            cerr << "Redis connection error: " << context->errstr << endl;
            redisFree(context);
            context = nullptr;
        }
        else
        {
            cerr << "Redis connection error: can't allocate redis context" << endl;
        }
        connected = false;
        return false;
    }

    // Test connection with PING
    redisReply *reply = (redisReply *)redisCommand(context, "PING");
    if (reply == nullptr)
    {
        cerr << "Redis PING failed" << endl;
        redisFree(context);
        context = nullptr;
        connected = false;
        return false;
    }

    freeReplyObject(reply);
    connected = true;
    cout << "Connected to Redis at " << host << ":" << port << endl;
    return true;
}

string RedisManager::get_task_key(int task_id) const
{
    return "task:" + to_string(task_id);
}

string RedisManager::serialize_task(const Task &task)
{
    json j;
    j["id"] = task.id;
    j["description"] = task.description;
    j["is_completed"] = task.is_completed;
    j["priority"] = task.priority;
    j["created_at"] = static_cast<long long>(task.created_at);

    if (task.due_date.has_value())
    {
        j["due_date"] = static_cast<long long>(task.due_date.value());
    }
    else
    {
        j["due_date"] = nullptr;
    }

    if (task.parent_id.has_value())
    {
        j["parent_id"] = task.parent_id.value();
    }
    else
    {
        j["parent_id"] = nullptr;
    }

    j["links"] = task.links;
    j["tags"] = task.tags;

    return j.dump();
}

optional<Task> RedisManager::deserialize_task(const string &json_str)
{
    try
    {
        json j = json::parse(json_str);

        Task task;
        task.id = j["id"].get<int>();
        task.description = j["description"].get<string>();
        task.is_completed = j["is_completed"].get<bool>();
        task.priority = j["priority"].get<int>();
        task.created_at = static_cast<time_t>(j["created_at"].get<long long>());

        if (!j["due_date"].is_null())
        {
            task.due_date = static_cast<time_t>(j["due_date"].get<long long>());
        }

        if (!j["parent_id"].is_null())
        {
            task.parent_id = j["parent_id"].get<int>();
        }

        task.links = j["links"].get<vector<string>>();
        task.tags = j["tags"].get<vector<int>>();

        return task;
    }
    catch (const std::exception &e)
    {
        cerr << "Error deserializing task: " << e.what() << endl;
        return nullopt;
    }
}

bool RedisManager::cache_task(const Task &task, int ttl)
{
    if (!connected)
    {
        return false;
    }

    string key = get_task_key(task.id);
    string value = serialize_task(task);

    redisReply *reply;
    if (ttl > 0)
    {
        reply = (redisReply *)redisCommand(context, "SETEX %s %d %s",
                                           key.c_str(), ttl, value.c_str());
    }
    else
    {
        reply = (redisReply *)redisCommand(context, "SET %s %s",
                                           key.c_str(), value.c_str());
    }

    if (reply == nullptr)
    {
        cerr << "Redis SET command failed" << endl;
        return false;
    }

    bool success = (reply->type == REDIS_REPLY_STATUS &&
                    string(reply->str) == "OK");

    freeReplyObject(reply);
    return success;
}

optional<Task> RedisManager::get_cached_task(int task_id)
{
    if (!connected)
    {
        cache_misses++;
        return nullopt;
    }

    string key = get_task_key(task_id);

    redisReply *reply = (redisReply *)redisCommand(context, "GET %s", key.c_str());

    if (reply == nullptr)
    {
        cerr << "Redis GET command failed" << endl;
        cache_misses++;
        return nullopt;
    }

    if (reply->type == REDIS_REPLY_STRING)
    {
        string json_str(reply->str, reply->len);
        freeReplyObject(reply);
        cache_hits++;
        return deserialize_task(json_str);
    }

    freeReplyObject(reply);
    cache_misses++;
    return nullopt;
}

bool RedisManager::invalidate_task(int task_id)
{
    if (!connected)
    {
        return false;
    }

    string key = get_task_key(task_id);

    redisReply *reply = (redisReply *)redisCommand(context, "DEL %s", key.c_str());

    if (reply == nullptr)
    {
        cerr << "Redis DEL command failed" << endl;
        return false;
    }

    bool success = (reply->type == REDIS_REPLY_INTEGER && reply->integer >= 0);
    freeReplyObject(reply);
    return success;
}

int RedisManager::cache_tasks(const vector<Task> &tasks, int ttl)
{
    int count = 0;
    for (const auto &task : tasks)
    {
        if (cache_task(task, ttl))
        {
            count++;
        }
    }
    return count;
}

bool RedisManager::clear_all_tasks()
{
    if (!connected)
    {
        return false;
    }

    // Delete all keys matching the pattern "task:*"
    redisReply *reply = (redisReply *)redisCommand(context, "KEYS task:*");

    if (reply == nullptr || reply->type != REDIS_REPLY_ARRAY)
    {
        cerr << "Redis KEYS command failed" << endl;
        if (reply)
            freeReplyObject(reply);
        return false;
    }

    for (size_t i = 0; i < reply->elements; i++)
    {
        redisReply *del_reply = (redisReply *)redisCommand(context, "DEL %s",
                                                           reply->element[i]->str);
        if (del_reply)
            freeReplyObject(del_reply);
    }

    freeReplyObject(reply);
    return true;
}

string RedisManager::get_stats() const
{
    stringstream ss;
    int total = cache_hits + cache_misses;
    float hit_rate = total > 0 ? (float)cache_hits / total * 100.0f : 0.0f;

    ss << "Cache Statistics:\n";
    ss << "  Hits: " << cache_hits << "\n";
    ss << "  Misses: " << cache_misses << "\n";
    ss << "  Total: " << total << "\n";
    ss << "  Hit Rate: " << hit_rate << "%\n";

    return ss.str();
}
