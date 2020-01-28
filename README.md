gitfs
=====
Simple and incomplete userspace filesystem to mount a git repository.

Ideal for repositories with multiple people working on multiple branches
at the same time and with a buildsystem that supports out-of-tree builds.
However, this software is provided with no warranty or suitability for a
specific purpose.

Features that work:
* Mounting a bare or normal repository
* Local and remote branches show up as symlinks to their respective commits
* Any commit can be 'cd'ed into and browsed as normal

Features the usage suggests but are not implemented/supported:
* Mounting the tip of a specific branch
* Mounting a specific commit
* Write access

License
=======
Created and written by Esther Dalhuisen.
I may add a license later, probably GPL-2. Until then it's public domain I suppose :)
