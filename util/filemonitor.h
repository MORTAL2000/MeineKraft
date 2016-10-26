#ifndef MEINEKRAFT_FILEMONITOR_H
#define MEINEKRAFT_FILEMONITOR_H

#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

class FileMonitor {
public:
    /// Watches all the files in all added to it
    FileMonitor();

    /// Starts checking files for modification
    void start_monitor();

    /// Ends checking files for modification
    void end_monitor();

    /// Adds a file to the FileMonitor if there exists such a file
    void add_file(std::string filepath);

    /// Clears the modification flag for one file
    void clear_modification_flag(std::string filepath);

    /// Clears all the modification flags for all the files
    void clear_all_modification_flags();

    /// Returns a vector of all the filepaths that was modified recently
    std::vector<std::string> get_modified_files();

    /// Is set to true whenever a file in the watch list is modified
    bool files_modfied;

private:
    bool monitoring;
    std::mutex internal_lock;
    void poll_files();

    /// Map of files with their modification flags
    std::unordered_map<std::string, bool> modified_files;

    /// Filepaths watched
    std::vector<std::string> watched_files;

    /// Filepaths and their modification time in seconds
    std::unordered_map<std::string, uint64_t> files_modification_time;
};

#endif //MEINEKRAFT_FILEMONITOR_H