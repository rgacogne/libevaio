
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <evaio.h>

#define LOG(...)                                                        \
    do                                                                  \
    {                                                                   \
        fprintf(stderr, "[%s(%d)] %s: ", __FILE__, __LINE__, __func__); \
        fprintf(stderr, __VA_ARGS__);                                   \
        fputs("\n", stderr);                                            \
    }                                                                   \
    while(0)

#define TEST_ASSERT(condition, message)                                 \
    if ((condition) == false)                                           \
    {                                                                   \
        LOG("Assertion {%s} failed in %s: %s", #condition,__func__, (message) ); \
    }

#define TEST_BUFFER_SIZE (20)

typedef struct
{
    struct event_base * base;
    bool completed;
} test_data;

static void test_completion_handler(evaio * aio,
                                    errno_t status,
                                    size_t transferred,
                                    void * user_data)
{
    assert(aio != NULL);
    assert(user_data != NULL);
    test_data * data = user_data;
    (void) aio;

    TEST_ASSERT(data->completed == false, "completion flag should be false");
    TEST_ASSERT(status == 0, "status should be 0");
    TEST_ASSERT(transferred == TEST_BUFFER_SIZE, "TEST_BUFFER_SIZE should have been transferred");

    if (status == 0 && transferred == TEST_BUFFER_SIZE)
    {
        data->completed = true;
    }
    else
    {
        LOG("status is %d", status);
        LOG("transferred is %zu", transferred);
    }
    event_base_loopbreak(data->base);
}

static int test_write(struct event_base * const base,
                      evaio * const aio)
{
    int result = 0;
    int fd = open("/tmp/evaio-test.tmp", O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
    assert(base != NULL);
    assert(aio != NULL);

    TEST_ASSERT(fd >= 0, "opening test file for writing");

    if (fd >= 0)
    {
        char buffer[TEST_BUFFER_SIZE] = "AAAAAAAAAAAAAAAAAAAA";
        size_t buffer_size = sizeof buffer;
        test_data data =
            {
                .completed = false,
                .base = base
            };
        evaio_config config =
            {
                .aio = aio,
                .data = buffer,
                .data_size = buffer_size,
                .cb = &test_completion_handler,
                .user_data = &data,
                .offset = 0,
                .fd = fd
            };

        errno_t res = evaio_write(&config);

        TEST_ASSERT(res == 0, "evaio_write result");

        if (res == 0)
        {
            result = event_base_dispatch(base);

            /* 1 here means that no event are registered, which is not good.
               -1 means a real error. */
            TEST_ASSERT(result == 0, "event_base_dispatch result");

            if (result == 0)
            {
                TEST_ASSERT(data.completed == true, "write completed");
            }
            else
            {
                LOG("event_base_dispatch failed with %d", result);
            }
        }
        else
        {
            result = res;
            LOG("evaio_write failed with %d", result);
        }

        close(fd), fd = -1;
    }
    else
    {
        result = errno;
        LOG("Opening test file failed with %d", result);
    }

    return result;
}

static int test_read(struct event_base * const base,
                     evaio * const aio)
{
    int result = 0;
    int fd = open("/dev/zero", O_RDONLY);
    assert(base != NULL);
    assert(aio != NULL);

    TEST_ASSERT(fd >= 0, "opening test file");

    if (fd >= 0)
    {
        char buffer[TEST_BUFFER_SIZE];
        size_t buffer_size = sizeof buffer;
        test_data data =
            {
                .completed = false,
                .base = base
            };
        evaio_config config =
            {
                .aio = aio,
                .data = buffer,
                .data_size = buffer_size,
                .cb = &test_completion_handler,
                .user_data = &data,
                .offset = 0,
                .fd = fd
            };

        errno_t res = evaio_read(&config);

        TEST_ASSERT(res == 0, "evaio_read result");

        if (res == 0)
        {
            result = event_base_dispatch(base);

            /* 1 here means that no event are registered, which is not good.
               -1 means a real error. */
            TEST_ASSERT(result == 0, "event_base_dispatch result");

            if (result == 0)
            {
                TEST_ASSERT(data.completed == true, "read completed");

                result = memcmp(buffer, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", buffer_size);

                TEST_ASSERT(result == 0, "read consistency");

            }
            else
            {
                LOG("event_base_dispatch failed with %d", result);
            }
        }
        else
        {
            result = res;
            LOG("evaio_read failed with %d", result);
        }

        close(fd), fd = -1;
    }
    else
    {
        result = errno;
        LOG("Opening test file failed with %d", result);
    }

    return result;
}

int main(void)
{
    int result = 0;

    struct event_base * base = event_base_new();

    if (base != NULL)
    {
        evaio * aio = evaio_new(base);

        TEST_ASSERT(aio != NULL, "evaio_new(base) should not return NULL");

        if (aio != NULL)
        {
            result = test_read(base, aio);

            TEST_ASSERT(result == 0, "test_read() should return 0");

            result = test_write(base, aio);

            TEST_ASSERT(result == 0, "test_write() should return 0");

            evaio_del(NULL);
            evaio_del(aio), aio = NULL;
        }
        else
        {
            result = ENOMEM;
            LOG("Allocation failed for event_base");
        }

        event_base_free(base), base = NULL;
    }
    else
    {
        result = ENOMEM;
        LOG("Allocation failed for event_base");
    }

    return result;
}
