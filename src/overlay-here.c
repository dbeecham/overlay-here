#define _GNU_SOURCE

#include <sched.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <sys/mount.h>
#include <unistd.h>
#include <stdio.h>
#include <grp.h>
#include <stdlib.h>
#include <sys/stat.h>


int main (
    int argc,
    char const* argv[]
)
{
    int ret = 0;
    char cwd[1024];
    char mount_target[1024];
    char mount_data[1024];
    char * c_ret = NULL;
    char * shell = NULL;

    openlog("overlay-here", LOG_NDELAY | LOG_PERROR, LOG_USER);


    // get the current working directory
    c_ret = getcwd(cwd, sizeof(cwd));
    if (NULL == c_ret) {
        syslog(LOG_ERR, "%s:%d:%s: getcwd: %s", __FILE__, __LINE__, __func__, strerror(errno));
        return -1;
    }


    // get the SHELL env variable
    shell = getenv("SHELL");
    if (NULL == shell) {
        syslog(LOG_ERR, "%s:%d:%s: SHELL env variable is not set", __FILE__, __LINE__, __func__);
        return -1;
    }


    // create the 'overlay' directory
    ret = mkdir("overlay", 0700);
    if (-1 == ret && EEXIST != errno) {
        syslog(LOG_ERR, "%s:%d:%s: mkdir: %s", __FILE__, __LINE__, __func__, strerror(errno));
        return -1;
    }

    ret = chown("overlay", getuid(), getgid());
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: chown: %s", __FILE__, __LINE__, __func__, strerror(errno));
        return -1;
    }

    // create the 'overlay/mnt' directory
    ret = mkdir("overlay/mnt", 0700);
    if (-1 == ret && EEXIST != errno) {
        syslog(LOG_ERR, "%s:%d:%s: mkdir: %s", __FILE__, __LINE__, __func__, strerror(errno));
        return -1;
    }

    ret = chown("overlay/mnt", getuid(), getgid());
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: chown: %s", __FILE__, __LINE__, __func__, strerror(errno));
        return -1;
    }

    ret = mkdir("overlay/workdir", 0700);
    if (-1 == ret && EEXIST != errno) {
        syslog(LOG_ERR, "%s:%d:%s: mkdir: %s", __FILE__, __LINE__, __func__, strerror(errno));
        return -1;
    }

    ret = chown("overlay/workdir", getuid(), getgid());
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: chown: %s", __FILE__, __LINE__, __func__, strerror(errno));
        return -1;
    }

    ret = mkdir("overlay/upperdir", 0700);
    if (-1 == ret && EEXIST != errno) {
        syslog(LOG_ERR, "%s:%d:%s: mkdir: %s", __FILE__, __LINE__, __func__, strerror(errno));
        return -1;
    }

    ret = chown("overlay/upperdir", getuid(), getgid());
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: chown: %s", __FILE__, __LINE__, __func__, strerror(errno));
        return -1;
    }


    // unshare mount namespace
    ret = unshare(CLONE_NEWNS);
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: unshare: %s", __FILE__, __LINE__, __func__, strerror(errno));
        return -1;
    }

    // set propagation
    ret = mount(
        /* src = */ "none",
        /* target = */ "/",
        /* fstype = */ NULL,
        /* flags = */ MS_PRIVATE | MS_REC,
        /* data = */ NULL
    );
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: mount: %s", __FILE__, __LINE__, __func__, strerror(errno));
        return -1;
    }


    // mount overlay
    ret = snprintf(mount_target, sizeof(mount_target), "%s/overlay/mnt", cwd);
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: snprintf returned -1", __FILE__, __LINE__, __func__);
        return -1;
    }

    ret = snprintf(mount_data, sizeof(mount_data), "upperdir=%s/overlay/upperdir,workdir=%s/overlay/workdir,lowerdir=/", cwd, cwd);
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: snprintf returned -1", __FILE__, __LINE__, __func__);
        return -1;
    }

    ret = mount(
        /* src = */ "overlay",
        /* target = */ mount_target,
        /* fstype = */ "overlay",
        /* flags = */ 0,
        /* data = */ mount_data
    );
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: mount: %s", __FILE__, __LINE__, __func__, strerror(errno));
        return -1;
    }


    // mount proc in the new chroot
    ret = snprintf(mount_target, sizeof(mount_target), "%s/overlay/mnt/proc", cwd);
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: snprintf returned -1", __FILE__, __LINE__, __func__);
        return -1;
    }

    ret = mount(
        /* src = */ "proc",
        /* target = */ mount_target,
        /* fstype = */ "proc",
        /* flags = */ 0,
        /* data = */ NULL
    );
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: mount: %s", __FILE__, __LINE__, __func__, strerror(errno));
        return -1;
    }


    // mount dev in the new chroot
    ret = snprintf(mount_target, sizeof(mount_target), "%s/overlay/mnt/dev", cwd);
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: snprintf returned -1", __FILE__, __LINE__, __func__);
        return -1;
    }

    ret = mount(
        /* src = */ "/dev",
        /* target = */ mount_target,
        /* fstype = */ 0,
        /* flags = */ MS_BIND,
        /* data = */ NULL
    );
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: mount: %s", __FILE__, __LINE__, __func__, strerror(errno));
        return -1;
    }


    // mount dev in the new chroot
    ret = snprintf(mount_target, sizeof(mount_target), "%s/overlay/mnt/dev", cwd);
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: snprintf returned -1", __FILE__, __LINE__, __func__);
        return -1;
    }

    ret = mount(
        /* src = */ "/dev",
        /* target = */ mount_target,
        /* fstype = */ 0,
        /* flags = */ MS_BIND,
        /* data = */ NULL
    );
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: mount: %s", __FILE__, __LINE__, __func__, strerror(errno));
        return -1;
    }


    // mount /nix/store in the new chroot
    ret = snprintf(mount_target, sizeof(mount_target), "%s/overlay/mnt/nix/store", cwd);
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: snprintf returned -1", __FILE__, __LINE__, __func__);
        return -1;
    }

    ret = mount(
        /* src = */ "/nix/store",
        /* target = */ mount_target,
        /* fstype = */ 0,
        /* flags = */ MS_BIND,
        /* data = */ NULL
    );
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: mount: %s", __FILE__, __LINE__, __func__, strerror(errno));
        return -1;
    }


    // mount /run in the new chroot
    ret = snprintf(mount_target, sizeof(mount_target), "%s/overlay/mnt/run", cwd);
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: snprintf returned -1", __FILE__, __LINE__, __func__);
        return -1;
    }

    ret = mount(
        /* src = */ "/run",
        /* target = */ mount_target,
        /* fstype = */ 0,
        /* flags = */ MS_BIND,
        /* data = */ NULL
    );
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: mount: %s", __FILE__, __LINE__, __func__, strerror(errno));
        return -1;
    }


    // make a new /run/user/pid in the target chroot
    ret = snprintf(mount_target, sizeof(mount_target), "%s/overlay/mnt/run/user/1000", cwd);
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: snprintf returned -1", __FILE__, __LINE__, __func__);
        return -1;
    }

    ret = mount(
        /* src = */ "/run/user/1000",
        /* target = */ mount_target,
        /* fstype = */ NULL,
        /* flags = */ MS_BIND,
        /* data = */ NULL
    );
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: mount: %s", __FILE__, __LINE__, __func__, strerror(errno));
        return -1;
    }


    // chroot into the container
    ret = snprintf(mount_target, sizeof(mount_target), "%s/overlay/mnt", cwd);
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: snprintf returned -1", __FILE__, __LINE__, __func__);
        return -1;
    }

    ret = chroot(mount_target);
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: chroot: %s", __FILE__, __LINE__, __func__, strerror(errno));
        return -1;
    }


    // chdir to the correct path
    ret = chdir(cwd);
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: chdir: %s", __FILE__, __LINE__, __func__, strerror(errno));
        return -1;
    }


    // drop ancillary groups
    ret = setgroups(1, &(gid_t){getgid()});
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: setgroups: %s", __FILE__, __LINE__, __func__, strerror(errno));
        return -1;
    }

    // drop egid and saved gid (don't use setgid here! also remember to drop
    // group privileges before dropping uid privileges!)
    ret = setregid(getgid(), getgid());
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: setegid: %s", __FILE__, __LINE__, __func__, strerror(errno));
        return -1;
    }

    // drop euid and saved uid (don't use setuid here!)
    ret = setreuid(getuid(), getuid());
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: seteuid: %s", __FILE__, __LINE__, __func__, strerror(errno));
        return -1;
    }


    // set the SHELL_PREFIX env variable
    ret = setenv(
        /* name = */ "SHELL_PREFIX",
        /* value = */ "(overlay) ",
        /* overwrite = */ 0
    );
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d:%s: setenv: %s", __FILE__, __LINE__, __func__, strerror(errno));
        return -1;
    }


    // exec a shell
    execlp(shell, shell, NULL);


    // exec failed
    syslog(LOG_ERR, "%s:%d:%s: execlp: %s", __FILE__, __LINE__, __func__, strerror(errno));
    return -1;
}
