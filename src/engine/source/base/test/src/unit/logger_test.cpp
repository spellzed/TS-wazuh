#include <cstdio>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <regex>

#include <base/logging.hpp>

std::string readFileContents(const std::string& filePath)
{
    std::ifstream fileStream(filePath);
    if (!fileStream.is_open())
    {
        std::cerr << "Error to open the file " << filePath << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << fileStream.rdbuf();
    return buffer.str();
}

class LoggerTest : public testing::Test
{
public:
    std::string m_tmpPath;
    void SetUp() override
    {
        char tempFileName[] = "/tmp/temp_log_XXXXXX";
        auto tempFileDescriptor = mkstemp(tempFileName);
        m_tmpPath = tempFileName;
        ASSERT_NE(tempFileDescriptor, -1);
    }

    void TearDown() override
    {
        logging::stop();
        std::filesystem::remove(m_tmpPath); // Remove temporary log file
    }
};

TEST_F(LoggerTest, LogNonExist)
{
    ASSERT_ANY_THROW(logging::setLevel(logging::Level::Info));
}

TEST_F(LoggerTest, LogSuccessStart)
{
    ASSERT_NO_THROW(logging::start(logging::LoggingConfig {.filePath = m_tmpPath, .level = logging::Level::Info}));
}

TEST_F(LoggerTest, LogRepeatedStart)
{
    ASSERT_NO_THROW(logging::start(logging::LoggingConfig {.filePath = m_tmpPath}));
    ASSERT_ANY_THROW(logging::start(logging::LoggingConfig {.filePath = m_tmpPath}));
}

TEST_F(LoggerTest, LogGetSomeInstance)
{
    ASSERT_NO_THROW(logging::start(logging::LoggingConfig {.filePath = m_tmpPath}));
    auto logger = logging::getDefaultLogger();
    auto someLogger = logging::getDefaultLogger();
    ASSERT_NE(logger, nullptr);
    ASSERT_EQ(logger, someLogger);
}

class LoggerTestLevels : public ::testing::TestWithParam<logging::Level>
{
public:
    std::string m_tmpPath;
    void SetUp() override
    {
        char tempFileName[] = "/tmp/temp_log_XXXXXX";
        auto tempFileDescriptor = mkstemp(tempFileName);
        m_tmpPath = tempFileName;
        ASSERT_NE(tempFileDescriptor, -1);
    }

    void checkLogFileContent(const std::string& message, bool shouldContain)
    {
        std::string fileContent = readFileContents(m_tmpPath);
        if (shouldContain)
        {
            EXPECT_NE(fileContent.find(message), std::string::npos);
        }
        else
        {
            EXPECT_EQ(fileContent.find(message), std::string::npos);
        }
    }

    bool shouldContainMessage(logging::Level currentLevel, logging::Level messageLevel)
    {
        return static_cast<int>(messageLevel) >= static_cast<int>(currentLevel);
    }

    void TearDown() override
    {
        logging::stop();
        std::filesystem::remove(m_tmpPath); // Remove temporary log file
    }
};

TEST_P(LoggerTestLevels, LogChangeLevelInRuntime)
{
    auto level = GetParam();

    ASSERT_NO_THROW(logging::start(logging::LoggingConfig {.filePath = m_tmpPath, .level = level}));

    LOG_TRACE("TRACE message");
    LOG_DEBUG("DEBUG message");
    LOG_INFO("INFO message");
    LOG_WARNING("WARNING message");
    LOG_ERROR("ERROR message");
    LOG_CRITICAL("CRITICAL message");

    checkLogFileContent("TRACE message", shouldContainMessage(level, logging::Level::Trace));
    checkLogFileContent("DEBUG message", shouldContainMessage(level, logging::Level::Debug));
    checkLogFileContent("INFO message", shouldContainMessage(level, logging::Level::Info));
    checkLogFileContent("WARNING message", shouldContainMessage(level, logging::Level::Warn));
    checkLogFileContent("ERROR message", shouldContainMessage(level, logging::Level::Err));
    checkLogFileContent("CRITICAL message", shouldContainMessage(level, logging::Level::Critical));
}

INSTANTIATE_TEST_CASE_P(Levels,
                        LoggerTestLevels,
                        ::testing::Values(logging::Level::Trace,
                                          logging::Level::Debug,
                                          logging::Level::Info,
                                          logging::Level::Warn,
                                          logging::Level::Err,
                                          logging::Level::Critical));

class LoggerTestExtraInfo : public ::testing::TestWithParam<std::tuple<logging::Level, std::regex>>
{
public:
    std::string m_tmpPath;
    void SetUp() override
    {
        char tempFileName[] = "/tmp/temp_log_XXXXXX";
        auto tempFileDescriptor = mkstemp(tempFileName);
        m_tmpPath = tempFileName;
        ASSERT_NE(tempFileDescriptor, -1);
    }

    void TearDown() override
    {
        logging::stop();
        std::filesystem::remove(m_tmpPath); // Remove temporary log file
    }
};

TEST_P(LoggerTestExtraInfo, LogPatternMatching)
{
    auto [level, pattern] = GetParam();

    ASSERT_NO_THROW(logging::start(logging::LoggingConfig {.filePath = m_tmpPath, .level = level}));

    LOG_TRACE("TRACE message");
    LOG_DEBUG("DEBUG message");
    LOG_INFO("INFO message");
    LOG_WARNING("WARNING message");
    LOG_ERROR("ERROR message");
    LOG_CRITICAL("CRITICAL message");

    std::istringstream iss(readFileContents(m_tmpPath));
    std::string line;
    while (std::getline(iss, line))
    {
        EXPECT_TRUE(std::regex_match(line, pattern));
    }
}

INSTANTIATE_TEST_CASE_P(
    LevelsWithRegex,
    LoggerTestExtraInfo,
    ::testing::Values(std::make_tuple(logging::Level::Trace,
                                      std::regex(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d+ \d+:\d+ .*: (\w+): .*)")),
                      std::make_tuple(logging::Level::Debug,
                                      std::regex(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d+ \d+:\d+ .*: (\w+): .*)")),
                      std::make_tuple(logging::Level::Info,
                                      std::regex(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d+ \d+:\d+ (\w+): .*)")),
                      std::make_tuple(logging::Level::Warn,
                                      std::regex(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d+ \d+:\d+ (\w+): .*)")),
                      std::make_tuple(logging::Level::Err,
                                      std::regex(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d+ \d+:\d+ (\w+): .*)")),
                      std::make_tuple(logging::Level::Critical,
                                      std::regex(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d+ \d+:\d+ (\w+): .*)"))));

class LoggerTestLevelsParam
    : public ::testing::TestWithParam<std::tuple<logging::Level, std::vector<std::string>, std::vector<std::string>>>
{
public:
    void TearDown() override { logging::stop(); }
};

TEST_P(LoggerTestLevelsParam, LogLevelTest)
{
    auto [level, expectedMessages, unexpectedMessages] = GetParam();
    ASSERT_NO_THROW(logging::start(logging::LoggingConfig {.level = level}));

    if (level >= logging::Level::Warn)
    {
        testing::internal::CaptureStderr();
    }
    else
    {
        testing::internal::CaptureStdout();
    }

    LOG_TRACE("TRACE message");
    LOG_DEBUG("DEBUG message");
    LOG_INFO("INFO message");
    LOG_WARNING("WARNING message");
    LOG_ERROR("ERROR message");
    LOG_CRITICAL("CRITICAL message");

    std::string output;
    if (level >= logging::Level::Warn)
    {
        output = testing::internal::GetCapturedStderr();
    }
    else
    {
        output = testing::internal::GetCapturedStdout();
    }

    for (const auto& message : expectedMessages)
    {
        EXPECT_NE(output.find(message), std::string::npos) << "Expected to find: " << message;
    }

    for (const auto& message : unexpectedMessages)
    {
        EXPECT_EQ(output.find(message), std::string::npos) << "Did not expect to find: " << message;
    }
}

INSTANTIATE_TEST_CASE_P(
    LevelsTest,
    LoggerTestLevelsParam,
    ::testing::Values(
        // Level Trace: only "INFO message" should appear
        std::make_tuple(logging::Level::Trace,
                        std::vector<std::string> {"INFO message"},
                        std::vector<std::string> {
                            "TRACE message", "DEBUG message", "WARNING message", "ERROR message", "CRITICAL message"}),

        // Level Trace: only "INFO message" should appear
        std::make_tuple(logging::Level::Debug,
                        std::vector<std::string> {"INFO message"},
                        std::vector<std::string> {
                            "TRACE message", "DEBUG message", "WARNING message", "ERROR message", "CRITICAL message"}),

        // Level Info: only "INFO message" should appear
        std::make_tuple(logging::Level::Info,
                        std::vector<std::string> {"INFO message"},
                        std::vector<std::string> {
                            "TRACE message", "DEBUG message", "WARNING message", "ERROR message", "CRITICAL message"}),

        // Level Critical: "CRITICAL message", "ERROR message", and "WARNING message" should appear
        std::make_tuple(logging::Level::Warn,
                        std::vector<std::string> {"WARNING message", "ERROR message", "CRITICAL message"},
                        std::vector<std::string> {"INFO message", "DEBUG message", "TRACE message"}),

        // Level Critical: "CRITICAL message", "ERROR message", and "WARNING message" should appear
        std::make_tuple(logging::Level::Err,
                        std::vector<std::string> {"WARNING message", "ERROR message", "CRITICAL message"},
                        std::vector<std::string> {"INFO message", "DEBUG message", "TRACE message"}),

        // Level Critical: "CRITICAL message", "ERROR message", and "WARNING message" should appear
        std::make_tuple(logging::Level::Critical,
                        std::vector<std::string> {"WARNING message", "ERROR message", "CRITICAL message"},
                        std::vector<std::string> {"INFO message", "DEBUG message", "TRACE message"})));
