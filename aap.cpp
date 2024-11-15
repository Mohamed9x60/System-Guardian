#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <sstream>
#include <map>
#include <unistd.h>
#include <sys/inotify.h>
#include <vector>
#include <dirent.h>

// اسم المبرمج
const std::string programmerName = "Mohamed Fouad";

// دالة لعرض البانر
void displayBanner() {
    std::cout << "============================\n";
    std::cout << "       System Guardian      \n";
    std::cout << "        Programmer: " << programmerName << "\n";
    std::cout << "============================\n\n";
    
    std::cout << "فلسطين حرة\n";
}

// دالة لتسجيل الأحداث في ملف يومي
void logEvent(const std::string &event) {
    // الحصول على التاريخ والوقت الحالي
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string dateTime = std::ctime(&now);
    dateTime.pop_back();  // إزالة السطر الجديد من نهاية الوقت
    
    // اسم ملف السجل اليومي بناءً على التاريخ
    std::string filename = "logs_" + dateTime.substr(0, 10) + ".txt"; // yyyy-mm-dd
    std::ofstream logFile(filename, std::ios::app);
    if (logFile.is_open()) {
        logFile << "[" << dateTime << "] " << event << "\n";
        logFile.close();
    } else {
        std::cerr << "Failed to open log file.\n";
    }
}

// دالة لمراقبة النظام (CPU، الذاكرة، العمليات)
void monitorSystem() {
    while (true) {
        logEvent("Checking system health...");
        
        // فحص استخدام المعالج
        std::ifstream cpuFile("/proc/stat");
        if (cpuFile.is_open()) {
            std::string line;
            std::getline(cpuFile, line);
            logEvent("CPU usage: " + line);
            cpuFile.close();
        }

        // فحص استخدام الذاكرة
        std::ifstream memFile("/proc/meminfo");
        if (memFile.is_open()) {
            std::string line;
            std::getline(memFile, line);
            logEvent("Memory usage: " + line);
            memFile.close();
        }

        // مراقبة العمليات ذات الاستهلاك العالي
        std::cout << "Checking for processes consuming high resources...\n";
        system("ps aux --sort=-%cpu | awk 'NR>1 && $3>10 {print $2 \" \" $3 \" \" $11}' >> system_processes.log");
        system("ps aux --sort=-%mem | awk 'NR>1 && $4>100 {print $2 \" \" $4 \" \" $11}' >> system_processes.log");

        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

// دالة لمراقبة تغييرات الملفات باستخدام inotify
void monitorFiles() {
    int fd = inotify_init();
    if (fd == -1) {
        std::cerr << "Failed to initialize inotify.\n";
        return;
    }

    // مراقبة جميع الملفات في النظام
    int watchDescriptor = inotify_add_watch(fd, "/", IN_MODIFY | IN_CREATE | IN_DELETE);
    if (watchDescriptor == -1) {
        std::cerr << "Failed to watch directory.\n";
        return;
    }

    char buffer[1024];
    while (true) {
        int length = read(fd, buffer, sizeof(buffer));
        if (length < 0) {
            std::cerr << "Error reading inotify events.\n";
            return;
        }

        for (int i = 0; i < length; i += sizeof(struct inotify_event) + ((struct inotify_event*)&buffer[i])->len) {
            struct inotify_event *event = (struct inotify_event*)&buffer[i];
            if (event->mask & IN_CREATE) {
                logEvent("File created: " + std::string(event->name));
            }
            if (event->mask & IN_DELETE) {
                logEvent("File deleted: " + std::string(event->name));
            }
            if (event->mask & IN_MODIFY) {
                logEvent("File modified: " + std::string(event->name));
            }
        }
    }

    close(fd);
}

// دالة لمراقبة الشبكة باستخدام الأمر `netstat`
void monitorNetwork() {
    while (true) {
        logEvent("Monitoring network activity...");
        system("netstat -tuln >> network_log.txt");
        std::this_thread::sleep_for(std::chrono::seconds(15));
    }
}

// دالة لحذف الملفات المؤقتة والغير مستخدمة
void cleanTemporaryFiles() {
    while (true) {
        std::cout << "Cleaning up temporary files...\n";

        // تحديد المسارات التي تحتوي على ملفات مؤقتة أو غير مستخدمة
        const std::vector<std::string> directories = {"/tmp", "/var/tmp", "/home/user/.cache"};

        for (const auto& dir : directories) {
            DIR *d = opendir(dir.c_str());
            if (d) {
                struct dirent *entry;
                while ((entry = readdir(d)) != NULL) {
                    std::string filePath = dir + "/" + entry->d_name;
                    if (entry->d_name[0] != '.' && (filePath.find(".bak") != std::string::npos || filePath.find(".tmp") != std::string::npos)) {
                        std::remove(filePath.c_str());
                        logEvent("Deleted temporary file: " + filePath);
                    }
                }
                closedir(d);
            }
        }

        // نوم لمدة 24 ساعة قبل تنظيف الملفات مجددًا
        std::this_thread::sleep_for(std::chrono::hours(24)); // تنظيف كل 24 ساعة
    }
}

// البرنامج الرئيسي
int main() {
    displayBanner();
    std::cout << "Starting system monitoring...\n\n";

    // تشغيل مراقبة النظام والملفات والشبكة في الخلفية
    std::thread monitorSystemThread(monitorSystem);
    std::thread monitorFilesThread(monitorFiles);
    std::thread monitorNetworkThread(monitorNetwork);
    std::thread cleanTempFilesThread(cleanTemporaryFiles);
    
    monitorSystemThread.detach();
    monitorFilesThread.detach();
    monitorNetworkThread.detach();
    cleanTempFilesThread.detach();
    
    // في هذه المرحلة، لا حاجة لواجهة الأوامر (CLI) لأن البرنامج يعمل في الخلفية
    // سيقوم البرنامج بمراقبة النظام تلقائيًا وتسجيل الأحداث.

    // إضافة نقطة توقف للمراقبة بعد فترة معينة
    std::this_thread::sleep_for(std::chrono::minutes(60));

    return 0;
}
