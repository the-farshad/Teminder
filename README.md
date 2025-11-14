# Teminder

**An intelligent, persistent terminal application for task scheduling and management.**

Teminder is a lightweight, responsive TUI (Text User Interface) designed for developers, system administrators, and professionals who require efficient terminal-based task management. The application provides a keyboard-driven interface for managing tasks and reminders, incorporating AI capabilities to enhance productivity.

All data is stored locally in a SQLite database, ensuring optimal performance, data privacy, and continuous availability.

## Key Features

* **Persistent & Local-First:** All tasks are automatically saved to a local SQLite database. Data remains on the local machine at all times.
* **Redis Caching:** Optional in-memory caching with Redis for enhanced task access performance.
* **Interactive Terminal UI:** A full-screen, responsive interface that provides comprehensive schedule oversight within the command line environment.
* **Task Links:** Capability to attach URLs and links to tasks for streamlined reference to documentation, tickets, pull requests, and related resources.
* **AI-Powered Assistant:** Integration with local `ollama` instance to:
  * Suggest subsequent actionable steps for complex tasks.
  * Generate summaries of daily or weekly schedules.
  * Facilitate the decomposition of large objectives into smaller, manageable sub-tasks.
* **Task Scheduling:** Organization of tasks with configurable priorities and due dates for effective time management.
* **Docker Support:** Simplified deployment through Docker and docker-compose for containerized environments.
* **Highly Customizable:** Configuration of AI models (e.g., `llama3`, `mistral`), Redis settings, custom prompts, and application settings via `config.json` file. Change the name config_sample.json to config.json to get started.

## Prerequisites

Before building Teminder, ensure you have the following installed:

* **C++ Compiler:** GCC 8+ or Clang 10+ with C++17 support
* **CMake:** Version 3.16 or higher
* **Git:** For cloning repositories
* **curl/libcurl:** For HTTP requests (usually pre-installed)
* **Redis (Optional):** For in-memory caching - [Install Redis](https://redis.io/docs/getting-started/)
* **Ollama (Optional):** For AI features - [Install Ollama](https://ollama.ai/)
* **Docker & Docker Compose (Optional):** For containerized deployment


### Installing Prerequisites

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install -y build-essential cmake git ninja-build meson pkg-config libssl-dev zlib1g-dev libpsl-dev libzstd-dev libnghttp2-dev libidn2-0-dev libhiredis-dev
```


**Fedora/RHEL:**

```bash
sudo dnf install gcc-c++ cmake git libcurl-devel
```

**macOS:**

```bash
brew install cmake curl redis
```

### Installing Redis (Optional)

For high-performance in-memory caching:

**Ubuntu/Debian:**

```bash
sudo apt install redis-server
sudo systemctl start redis-server
sudo systemctl enable redis-server
```

**Fedora/RHEL:**

```bash
sudo dnf install redis
sudo systemctl start redis
sudo systemctl enable redis
```

**macOS:**

```bash
brew install redis
brew services start redis
```

### Installing Ollama (Optional)

For AI features, install Ollama and pull a model:

```bash
# Install Ollama (Linux)
curl -fsSL https://ollama.ai/install.sh | sh

# Pull a model (e.g., llama3)
ollama pull llama3

# Start Ollama service (if not auto-started)
ollama serve
```

## Building from Source

### Quick Build

```bash
# Clone the repository
git clone https://github.com/the-farshad/Temidner.git
cd Temidner

# Make build script executable
chmod +x build.sh

# Build
./build.sh

# Run
./build/Teminder
```

### Manual Build

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build (use -j for parallel builds)
cmake --build . -j$(nproc)

# Run
./Teminder
```

## Configuration

Teminder uses a `config.json` file for configuration. On first run, it will use default settings if the file is not found.

### Example config.json

```json
{
  "ai": {
    "enabled": true,
    "ollama_endpoint": "http://localhost:11434",
    "model_name": "llama3",
    "max_tokens": 500,
    "temperature": 0.7
  },
  "prompts": {
    "task_suggestion": "You are a helpful task management assistant. Analyze the following task and suggest the next actionable steps to complete it. Be concise and practical.",
    "summary": "You are a helpful task management assistant. Summarize the following tasks and provide a brief overview of what needs to be done. Highlight any overdue or high-priority items."
  },
  "database": {
    "path": "tasks.db"
  },
  "redis": {
    "enabled": true,
    "host": "localhost",
    "port": 6379
  }
}
```

### Configuration Options

| Option | Description | Default |
|--------|-------------|---------|
| `ai.enabled` | Enable/disable AI features | `true` |
| `ai.ollama_endpoint` | Ollama API endpoint | `http://localhost:11434` |
| `ai.model_name` | AI model to use | `llama3` |
| `ai.max_tokens` | Maximum tokens in AI response | `500` |
| `ai.temperature` | AI creativity (0.0-1.0) | `0.7` |
| `database.path` | Path to SQLite database | `tasks.db` |
| `redis.enabled` | Enable/disable Redis caching | `false` |
| `redis.host` | Redis server hostname | `localhost` |
| `redis.port` | Redis server port | `6379` |

## Usage

### Keyboard Shortcuts

| `redis.enabled` | Enable/disable Redis caching | `false` |
| `redis.host` | Redis server host | `localhost` |
| `redis.port` | Redis server port | `6379` |

## Usage

### Keyboard Shortcuts

#### Navigation

* `↑` or `k` - Move up in task list

* `↓` or `j` - Move down in task list

#### Task Management

* `a` - Add a new task

* `Space` - Toggle task completion status
* `d` - Delete selected task
* `p` - Cycle task priority (Low → Medium → High → Low)
* `c` - Toggle showing completed tasks
* `r` - Refresh task list

#### AI Features

* `s` - Get AI suggestions for selected task

* `Shift+S` - Get AI summary of all tasks
* `x` - Close AI panel

#### General

* `q` or `Esc` - Quit application

### Task Priority Levels

Tasks have three priority levels, indicated by colored indicators:

* 🟢 **Low Priority** (Green)
* 🟡 **Medium Priority** (Yellow)
* 🔴 **High Priority** (Red)

### Task Status Indicators

* `[ ]` - Incomplete task
* `[✓]` - Completed task
* `📅` - Task has a due date
* `🔗` - Task has attached links
* `⚠️ OVERDUE` - Task is past its due date

## Task Links

The application supports attaching URLs and links to tasks for efficient access to related resources:

* **Documentation links** - References to relevant documentation
* **Issue trackers** - GitHub issues, Jira tickets, and similar tracking systems
* **Pull requests** - Related pull requests and code reviews
* **Resources** - Articles, tutorials, and specifications
* **Meeting notes** - Associated discussions and decisions

Links are stored in SQLite for persistence and optionally cached in Redis for optimized access performance when enabled.

## AI Features

### Task Suggestions

Pressing `s` on any task generates AI-powered suggestions for subsequent steps. The AI analyzes the task description, priority level, and due date to provide actionable recommendations.

### Schedule Summary

Pressing `Shift+S` generates an AI-powered summary of all tasks. The summary includes:

* Overdue tasks
* High-priority items
* Overall workload assessment
* Task prioritization recommendations

### Requirements

* Ollama service must be running (`ollama serve`)
* An appropriate model must be available (e.g., `ollama pull llama3`)
* AI functionality must be enabled in `config.json`

### Troubleshooting AI Features

If AI features are not functioning as expected:

1. **Verify Ollama service status:**

   ```bash
   curl http://localhost:11434/api/tags
   ```

2. **Confirm model availability:**

   ```bash
   ollama list
   ```

3. **Review configuration settings:**
   * Ensure `ai.enabled` is set to `true` in `config.json`
   * Verify the accuracy of the `ollama_endpoint` value
   * Confirm the `model_name` corresponds to an installed model

## Database

Tasks are stored in a local SQLite database (`tasks.db` by default) with optional Redis caching for improved performance.

### SQLite Schema

The database includes two tables:

**tasks table:**

* Task description
* Completion status
* Priority level (0=Low, 1=Medium, 2=High)
* Creation timestamp
* Optional due date

**task_links table:**

* Task ID reference
* URL/link

### Database Schema

```sql
CREATE TABLE tasks (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    description  TEXT    NOT NULL,
    is_completed INTEGER NOT NULL DEFAULT 0,
    priority     INTEGER NOT NULL DEFAULT 0,
    created_at   INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    due_date     INTEGER
);

CREATE TABLE task_links (
    id      INTEGER PRIMARY KEY AUTOINCREMENT,
    task_id INTEGER NOT NULL,
    link    TEXT    NOT NULL,
    FOREIGN KEY (task_id) REFERENCES tasks(id) ON DELETE CASCADE,
    UNIQUE(task_id, link)
);
```

### Redis Caching

When Redis is enabled, tasks are cached in memory for fast access:

* **Task data**: Stored as JSON in `task:<id>` keys
* **Task links**: Stored as lists in `task:<id>:links` keys
* **Task IDs**: Maintained in a set at `tasks:ids`

Caching provides:

* **Faster reads**: Sub-millisecond task retrieval
* **Reduced database load**: Fewer SQLite queries
* **Scalability**: Better performance with large task lists

### Data Backup Procedures

To create a backup of task data:

```bash
# Simple file copy
cp tasks.db tasks.db.backup

# Alternative: SQLite dump
sqlite3 tasks.db .dump > tasks_backup.sql
```

## Architecture

Teminder is built with a modular architecture:

* **main.cpp** - Application entry point
* **DatabaseManager** - SQLite database operations and CRUD
* **ConfigManager** - Configuration file parsing and management
* **AIAssistant** - Ollama API integration for AI features
* **TaskListView** - FTXUI-based terminal user interface
* **Task** - Task data model

### Dependencies

Teminder uses the following libraries (automatically fetched by CMake):

* [FTXUI](https://github.com/ArthurSonzogni/FTXUI) - Terminal UI framework
* [SQLiteCpp](https://github.com/SRombauts/SQLiteCpp) - SQLite C++ wrapper
* [nlohmann/json](https://github.com/nlohmann/json) - JSON parsing
* [cpr](https://github.com/libcpr/cpr) - HTTP client library
* [hiredis](https://github.com/redis/hiredis) - Redis C client library

## Development

### Project Structure

```text
Temidner/
├── CMakeLists.txt          # Build configuration
├── build.sh                # Build script
├── config.json             # Configuration file
├── Dockerfile              # Docker build configuration
├── docker-compose.yml      # Docker services orchestration
├── .dockerignore          # Docker ignore patterns
├── README.md               # This file
├── LICENSE                 # License information
├── main.cpp                # Application entry point
├── Task.h                  # Task data structure
├── DatabaseManager.h/.cpp  # SQLite database operations
├── RedisManager.h/.cpp     # Redis caching operations
├── ConfigManager.h/.cpp    # Configuration management
├── AIAssistant.h/.cpp      # AI integration
└── TaskListView.h/.cpp     # Terminal UI
```

### Contributing

Contributions to this project are welcome. To submit a contribution:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/FeatureName`)
3. Commit changes with descriptive messages (`git commit -m 'Add feature description'`)
4. Push to the branch (`git push origin feature/FeatureName`)
5. Submit a Pull Request for review

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

* Built with [FTXUI](https://github.com/ArthurSonzogni/FTXUI) for terminal interface implementation
* AI functionality powered by [Ollama](https://ollama.ai/)
* Developed to address the need for efficient terminal-based task management solutions

## Support

For issues or inquiries:

1. Consult the [Troubleshooting](#troubleshooting-ai-features) section
2. Submit an issue on GitHub
3. Verify that all prerequisites are properly installed
