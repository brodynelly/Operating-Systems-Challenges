#include <cstdlib>
#include <iostream>
#include <new>
#include <vector>
using std::vector;
using std::string;
#include <gtest/gtest.h>
extern "C" 
{
#include "FS.h"
}

extern unsigned int score;
extern unsigned int total;

class GradeEnvironment : public testing::Environment 
{
    public:
        virtual void SetUp() 
        {
            score = 0;

            total = 50;
        }

        virtual void TearDown() 
        {
            ::testing::Test::RecordProperty("points_given", score);
            ::testing::Test::RecordProperty("points_total", total);
            std::cout << "SCORE: " << score << '/' << total << std::endl;
        }
};


bool find_in_directory(const dyn_array_t *const record_arr, const char *fname) 
{
    if (record_arr && fname) 
    {
        for (size_t i = 0; i < dyn_array_size(record_arr); ++i) 
        {
            if (strncmp(((file_record_t *) dyn_array_at(record_arr, i))->name, fname, FS_FNAME_MAX) == 0) 
            {
                return true;
            }
        }
    }
    return false;
}



/*
   FS * fs_format(const char *const fname);
   1   Normal
   2   NULL
   3   Empty string
   FS *fs_mount(const char *const fname);
   1   Normal
   2   NULL
   3   Empty string
   int fs_unmount(FS *fs);
   1   Normal
   2   NULL
 */
TEST(a_tests, format_mount_unmount) 
{
    const char *test_fname = "a_tests.FS";
    FS *fs = NULL;
    // FORMAT 2
    ASSERT_EQ(fs_format(NULL), nullptr);
    score++;

    // FORMAT 3
    // this really should just be caught by block_store
    ASSERT_EQ(fs_format(""), nullptr);
    score++;

    // FORMAT 1
    fs = fs_format(test_fname);
    ASSERT_NE(fs, nullptr);
    score++;

    // UNMOUNT 1
    ASSERT_EQ(fs_unmount(fs), 0);
    score++;

    // UNMOUNT 2
    ASSERT_LT(fs_unmount(NULL), 0);
    score++;

    // MOUNT 1
    fs = fs_mount(test_fname);
    ASSERT_NE(fs, nullptr);
    fs_unmount(fs);
    score++;

    // MOUNT 2
    ASSERT_EQ(fs_mount(NULL), nullptr);
    score++;

    // MOUNT 3
    ASSERT_EQ(fs_mount(""), nullptr);
    score++;
}

/*
   int fs_create(FS *const fs, const char *const fname, const ftype_t ftype);
   1. Normal, file, in root
   2. Normal, directory, in root
   3. Normal, file, not in root
   4. Normal, directory, not in root
   5. Error, NULL fs
   6. Error, NULL fname
   7. Error, empty fname
   8. Error, bad type
   9. Error, path does not exist
   10. Error, Root clobber
   11. Error, already exists
   12. Error, file exists
   13. Error, part of path not directory
   14. Error, path terminal not directory
   15. Error, path string has no leading slash
   16. Error, path has trailing slash (no name for desired file)
   17. Error, bad path, path part too long
   18. Error, bad path, desired filename too long
   19. Error, directory full.
   20. Error, out of inodes.
   21. Error, out of data blocks & file is directory (requires functional write)
 */
TEST(b_tests, file_creation_one) 
{
    vector<const char *> filenames
    {
        "/file", 
        "/folder", 
        "/folder/with_file", 
        "/folder/with_folder", 
        "/DOESNOTEXIST", 
        "/file/BAD_REQUEST",
        "/DOESNOTEXIST/with_file", 
        "/folder/with_file/bad_req", 
        "folder/missing_slash", 
        "/folder/new_folder/",
        "/folder/withwaytoolongfilenamethattakesupmorespacethanitshould and yet was not enough so I had to add "
        "more to make sure it was actually too long of a file name used as a directory name in truth/bad_req",
        "/folder/withfilethatiswayyyyytoolongwhydoyoumakefilesthataretoobigEXACT!withfilethatiswayyyyytoolongwhydoyoumakefilesthataretoobigEXACT!", 
        "/", 
        "/mystery_file"
    };
    const char *test_fname = "b_tests_normal.FS";
    FS *fs = fs_format(test_fname);
    ASSERT_NE(fs, nullptr);

    // CREATE_FILE 1
    ASSERT_EQ(fs_create(fs, filenames[0], FS_REGULAR), 0);
    score++;

    // CREATE_FILE 2
    ASSERT_EQ(fs_create(fs, filenames[1], FS_DIRECTORY), 0);
    score++;


    // CREATE_FILE 3
    ASSERT_EQ(fs_create(fs, filenames[2], FS_REGULAR), 0);
    score++;


    // CREATE_FILE 4
    ASSERT_EQ(fs_create(fs, filenames[3], FS_DIRECTORY), 0);
    score++;


    // CREATE_FILE 5
    ASSERT_LT(fs_create(NULL, filenames[4], FS_REGULAR), 0);
    score++;

    // CREATE_FILE 6
    ASSERT_LT(fs_create(fs, NULL, FS_REGULAR), 0);
    score++;


    // CREATE_FILE 7
    ASSERT_LT(fs_create(fs, "", FS_REGULAR), 0);
    score++;


    // CREATE_FILE 8
    ASSERT_LT(fs_create(fs, filenames[13], (file_t) 44), 0);
    score++;


    // CREATE_FILE 9
    ASSERT_LT(fs_create(fs, filenames[6], FS_REGULAR), 0);
    score++;


    // CREATE_FILE 10
    ASSERT_LT(fs_create(fs, filenames[12], FS_DIRECTORY), 0);
    score++;


    // CREATE_FILE 11
    ASSERT_LT(fs_create(fs, filenames[1], FS_DIRECTORY), 0);
    ASSERT_LT(fs_create(fs, filenames[1], FS_REGULAR), 0);
    score++;


    // CREATE_FILE 12
    ASSERT_LT(fs_create(fs, filenames[0], FS_REGULAR), 0);
    ASSERT_LT(fs_create(fs, filenames[0], FS_DIRECTORY), 0);
    score++;


    // CREATE_FILE 13
    ASSERT_LT(fs_create(fs, filenames[5], FS_REGULAR), 0);
    score++;


    // CREATE_FILE 14
    ASSERT_LT(fs_create(fs, filenames[7], FS_REGULAR), 0);
    score++;


    // CREATE_FILE 15
    ASSERT_LT(fs_create(fs, filenames[8], FS_REGULAR), 0);
    score++;

    // But if we don't support relative paths, is there a reason to force abolute notation?
    // It's really a semi-arbitrary restriction
    // I suppose relative paths are up to the implementation, since . and .. are just special folder entires
    // but that would mess with the directory content total, BUT extra parsing can work around that.
    // Hmmmm.
    // CREATE_FILE 16
    ASSERT_LT(fs_create(fs, filenames[9], FS_DIRECTORY), 0);
    score++;


    // CREATE_FILE 17
    ASSERT_LT(fs_create(fs, filenames[10], FS_REGULAR), 0);
    score++;


    // CREATE_FILE 18
    ASSERT_LT(fs_create(fs, filenames[11], FS_REGULAR), 0);
    score++;


    // Closing this file now for inspection to make sure these tests didn't mess it up
    fs_unmount(fs);
}

TEST(b_tests, file_creation_two) 
{
    // CREATE_FILE 19 - OUT OF INODES (and test 18 along the way)
    // Gotta make... Uhh... A bunch of files. (255, but we'll need directories to hold them as well)
    const char *test_fname = "b_tests_full_table.FS";
    FS *fs            = fs_format(test_fname);
    ASSERT_NE(fs, nullptr);
    // puts("Attempting to fill inode table...");
    // Dummy string to loop with
    char fname[] = "/a/a\0\0\0\0\0\0\0\0\0\0\0";  // extra space because this is all sorts of messed up now

    // This should get us to 224 inodes (7 directories + 7 * 31 files)
    for (char dir = 'a'; dir < 'h'; ++dir) 
    {
        fname[1] = dir;
        fname[2] = '\0';
        //printf("Directory: %s\n", fname);
        ASSERT_EQ(fs_create(fs, fname, FS_DIRECTORY), 0);

        fname[2] = '/';
        int counter = 0;
        for (unsigned char file = 'a'; file < ('a'+31); ++file) 
        {
            char realname;
            realname = file;
            if (realname>'z')
            {
                realname = 'A'+(realname-'z'-1);
            }
            fname[3] = realname;
            //printf("File: %s\n", fname);
            ASSERT_EQ(fs_create(fs, fname, FS_DIRECTORY), 0);
            counter++;
            //printf("%d files in dir\n",counter);
        }
    }

    // CREATE_FILE 19
    ASSERT_LT(fs_create(fs, "/a/F", FS_DIRECTORY), 0);
    score++;
    
    // Start making files to use up the remaining 31 inodes
    fname[0] = '/';
    fname[1] = 'a';
    fname[2] = '/';
    fname[3] = 'a';
    fname[4] = '/';
    for (unsigned char file = 'a'; file < ('a'+31); ++file) 
    {
        char realname = file;
        if (realname>'z')
        {
            realname = 'A'+(realname-'z'-1);
        }
        fname[5] = realname;
        //printf("File: %s\n", fname);
        ASSERT_EQ(fs_create(fs, fname, FS_REGULAR), 0);
    }
    score++;

    //puts("Inode table full?");
    // CREATE_FILE 20
    fname[0] = '/';
    fname[1] = 'e';
    fname[2] = '/';
    fname[3] = 'c';
    fname[4] = '/';
    fname[5] = 'f';
    ASSERT_LT(fs_create(fs, fname, FS_REGULAR), 0);
    score++;
    // save file for inspection
    fs_unmount(fs);
    // ... Can't really test 21 yet.
}

/*
   int fs_open(FS *fs, const char *path)
   1. Normal, file at root
   2. Normal, file in subdir
   3. Normal, multiple fd to the same file
   4. Error, NULL fs
   5. Error, NULL fname
   6. Error, empty fname ???
   7. Error, not a regular file
   8. Error, file does not exist
   9. Error, out of descriptors
   int fs_close(FS *fs, int fd);
   1. Normal, whatever
   2. Normal, attempt to use after closing, assert failure **
   3. Normal, multiple opens, close does not affect the others **
   4. Error, FS null
   5. Error, invalid fd, positive
   6. Error, invalid fd, positive, out of bounds
   7. Error, invaid fs, negative
 */
TEST(c_tests, open_close_file) 
{
    vector<const char *> filenames
    {
        "/file", 
        "/folder", 
        "/folder/with_file", 
        "/folder/with_folder", 
        "/DOESNOTEXIST", 
        "/file/BAD_REQUEST",
        "/DOESNOTEXIST/with_file", 
        "/folder/with_file/bad_req", 
        "folder/missing_slash", 
        "/folder/new_folder/",
        "/folder/withwaytoolongfilenamethattakesupmorespacethanitshould and yet was not enough so I had to add "
        "more to make sure it was actually too long of a file name used as a directory name in truth/bad_req",
        "/folder/withfilethatiswayyyyytoolongwhydoyoumakefilesthataretoobigEXACT!withfilethatiswayyyyytoolongwhydoyoumakefilesthataretoobigEXACT!", 
        "/", 
        "/mystery_file"
    };
    const char *test_fname = "c_tests.FS";
    ASSERT_EQ(system("cp b_tests_normal.FS c_tests.FS"), 0);
    FS *fs = fs_mount(test_fname);
    ASSERT_NE(fs, nullptr);
    int fd_array[256] = {-1};

    // OPEN_FILE 1
    fd_array[0] = fs_open(fs, filenames[0]);
    ASSERT_GE(fd_array[0], 0);
    score++;


    // CLOSE_FILE 4
    ASSERT_LT(fs_close(NULL, fd_array[0]), 0);
    score++;


    // CLOSE_FILE 1
    ASSERT_EQ(fs_close(fs, fd_array[0]), 0);
    score++;


    // CLOSE_FILE 2 and 3 elsewhere
    // CLOSE_FILE 5
    ASSERT_LT(fs_close(fs, 70), 0);
    score++;


    // CLOSE_FILE 6
    ASSERT_LT(fs_close(fs, 7583), 0);
    score++;


    // CLOSE_FILE 7
    ASSERT_LT(fs_close(fs, -18), 0);
    score++;

    // OPEN_FILE 2
    fd_array[1] = fs_open(fs, filenames[2]);
    ASSERT_GE(fd_array[1], 0);
    ASSERT_EQ(fs_close(fs, fd_array[0]), 0);
    score++;


    // OPEN_FILE 3
    fd_array[2] = fs_open(fs, filenames[0]);
    ASSERT_GE(fd_array[2], 0);
    fd_array[3] = fs_open(fs, filenames[0]);
    ASSERT_GE(fd_array[3], 0);
    fd_array[4] = fs_open(fs, filenames[0]);
    ASSERT_GE(fd_array[4], 0);
    ASSERT_EQ(fs_close(fs, fd_array[2]), 0);
    ASSERT_EQ(fs_close(fs, fd_array[3]), 0);
    ASSERT_EQ(fs_close(fs, fd_array[4]), 0);
    score++;


    // OPEN_FILE 4
    fd_array[5] = fs_open(NULL, filenames[0]);
    ASSERT_LT(fd_array[5], 0);
    score++;


    // OPEN_FILE 5
    fd_array[5] = fs_open(fs, NULL);
    ASSERT_LT(fd_array[5], 0);
    score++;


    // OPEN_FILE 6
    // Uhh, bad filename? Not a slash?
    // It's wrong for a bunch of reasons, really.
    fd_array[5] = fs_open(fs, "");
    ASSERT_LT(fd_array[5], 0);
    score++;

    // OPEN_FILE 7
    fd_array[5] = fs_open(fs, "/");
    ASSERT_LT(fd_array[5], 0);
    fd_array[5] = fs_open(fs, filenames[1]);
    ASSERT_LT(fd_array[5], 0);
    score++;


    // OPEN_FILE 8
    fd_array[5] = fs_open(fs, filenames[6]);
    ASSERT_LT(fd_array[5], 0);
    score++;

    // OPEN_FILE 9
    // In case I'm leaking descriptors, wipe them all
    fs_unmount(fs);
    fs = fs_mount(test_fname);
    ASSERT_NE(fs, nullptr);
    for (int i = 0; i < 256; ++i) 
    {
        fd_array[i] = fs_open(fs, filenames[0]);
    }
    int err = fs_open(fs, filenames[0]);
    ASSERT_LT(err, 0);
    fs_unmount(fs);
    score++;
}

/*
   int fs_get_dir(const FS *const fs, const char *const fname, dir_rec_t *const records)
   1. Normal, root I guess?
   2. Normal, subdir somewhere
   3. Normal, empty dir
   4. Error, bad path
   5. Error, NULL fname
   6. Error, NULL fs
   7. Error, not a directory
 */
TEST(f_tests, get_dir) 
{
   vector<const char *> fnames
   {
   "/file", "/folder", "/folder/with_file", "/folder/with_folder", "/DOESNOTEXIST", "/file/BAD_REQUEST",
   "/DOESNOTEXIST/with_file", "/folder/with_file/bad_req", "folder/missing_slash", "/folder/new_folder/",
   "/folder/withwaytoolongfilenamethattakesupmorespacethanitshould and yet was not enough so I had to add "
   "more/bad_req",
   "/folder/withfilethatiswayyyyytoolongwhydoyoumakefilesthataretoobigEXACT!", "/", "/mystery_file"
   };
   const char *test_fname = "f_tests.FS";
   ASSERT_EQ(system("cp c_tests.FS f_tests.FS"), 0);
   FS *fs = fs_mount(test_fname);
   ASSERT_NE(fs, nullptr);

    // FS_GET_DIR 1
    dyn_array_t *record_results = fs_get_dir(fs, "/");
    ASSERT_NE(record_results, nullptr);
    ASSERT_TRUE(find_in_directory(record_results, "file"));
    ASSERT_TRUE(find_in_directory(record_results, "folder"));
    ASSERT_EQ(dyn_array_size(record_results), 2);
    score++;
    dyn_array_destroy(record_results);


    // FS_GET_DIR 2
    record_results = fs_get_dir(fs, fnames[1]);
    ASSERT_NE(record_results, nullptr);
    ASSERT_TRUE(find_in_directory(record_results, "with_file"));
    ASSERT_TRUE(find_in_directory(record_results, "with_folder"));
    ASSERT_EQ(dyn_array_size(record_results), 2);
    score++;
    dyn_array_destroy(record_results);

    // FS_GET_DIR 3
    record_results = fs_get_dir(fs, fnames[3]);
    ASSERT_NE(record_results, nullptr);
    ASSERT_EQ(dyn_array_size(record_results), 0);
    score++;
    dyn_array_destroy(record_results);


    // FS_GET_DIR 4
    record_results = fs_get_dir(fs, fnames[9]);
    ASSERT_EQ(record_results, nullptr);
    score++;

    // FS_GET_DIR 5
    record_results = fs_get_dir(fs, NULL);
    ASSERT_EQ(record_results, nullptr);
    score++;


    // FS_GET_DIR 6
    record_results = fs_get_dir(NULL, fnames[3]);
    ASSERT_EQ(record_results, nullptr);
    score++;


    // FS_GET_DIR 7
    record_results = fs_get_dir(fs, fnames[0]);
    ASSERT_EQ(record_results, nullptr);
    score++;
    fs_unmount(fs);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new GradeEnvironment);
    return RUN_ALL_TESTS();
}
