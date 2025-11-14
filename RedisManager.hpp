#pragma once

#include <string>
#include <vector>
#include <optional>
#include <hiredis.h>
#include "Task.hpp"

using std::optional;
using std::string;
using std::vector;

/**
 * @brief Manages Redis caching for tasks
 * Provides high-performance in-memory caching for frequently accessed tasks
 */
class RedisManager
{
public:
    /**
     * @brief Constructor
     * @param host Redis server host (default: "localhost")
     * @param port Redis server port (default: 6379)
     */
    RedisManager(const string &host = "localhost", int port = 6379);

    /**
     * @brief Destructor - cleanup Redis connection
     */
    ~RedisManager();

    /**
     * @brief Connect to Redis server
     * @return true if successful, false otherwise
     */
    bool connect();

    /**
     * @brief Check if connected to Redis
     * @return true if connected, false otherwise
     */
    bool is_connected() const { return connected; }

    /**
     * @brief Cache a task in Redis
     * @param task The task to cache
     * @param ttl Time-to-live in seconds (0 = no expiration)
     * @return true if successful, false otherwise
     */
    bool cache_task(const Task &task, int ttl = 300);

    /**
     * @brief Get a cached task from Redis
     * @param task_id The ID of the task
     * @return Optional containing the task if found in cache
     */
    optional<Task> get_cached_task(int task_id);

    /**
     * @brief Invalidate (remove) a cached task
     * @param task_id The ID of the task to remove from cache
     * @return true if successful, false otherwise
     */
    bool invalidate_task(int task_id);

    /**
     * @brief Cache multiple tasks
     * @param tasks Vector of tasks to cache
     * @param ttl Time-to-live in seconds (0 = no expiration)
     * @return Number of tasks successfully cached
     */
    int cache_tasks(const vector<Task> &tasks, int ttl = 300);

    /**
     * @brief Clear all cached tasks
     * @return true if successful, false otherwise
     */
    bool clear_all_tasks();

    /**
     * @brief Get cache statistics
     * @return String with cache hit/miss statistics
     */
    string get_stats() const;

private:
    /**
     * @brief Serialize a task to JSON string
     * @param task The task to serialize
     * @return JSON string representation
     */
    string serialize_task(const Task &task);

    /**
     * @brief Deserialize a task from JSON string
     * @param json_str The JSON string
     * @return Optional containing the task if deserialization successful
     */
    optional<Task> deserialize_task(const string &json_str);

    /**
     * @brief Generate Redis key for a task
     * @param task_id The task ID
     * @return Redis key string
     */
    string get_task_key(int task_id) const;

    redisContext *context;
    string host;
    int port;
    bool connected;

    // Statistics
    mutable int cache_hits;
    mutable int cache_misses;
};
