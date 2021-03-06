# D & I of FreeBSD Chapter 1 Notes

Many of the design ideas seen in Unix were originally from other operating
systems, meaning that Unix itself was a synthesis of previous work.

Example 1: BCPL --> B language --> C language

Example 2: The terminal drivers and virtual memory interface for BSD first appeared in TENEX/TOPS-20

Unix was special/first in three ways:

  1. It was written in a high-level language vs assembly
  2. It was distributed in source form to other research orgs
  3. Provided powerful primitives without needing expensive hardware

Around the time Unix came out, only operating systems on large expensive machines could run concurrent processes.

The first Unix system with portability as a design goal was V7, which ran on PDP-11 and Interdata 8/32.

William Joy and Ozalp Babaoglu's addition of virtual memory, demand paging, and page replacement to the Unix Support Group's V7 VAX Unix is what prompted DARPA to fund Berkeley with the implementation of TCP/IP protocols. Demand paging was originally added to Unix to support large programs.

Multics has often been a reference point in the design of new facilities in BSD

Unix variants adopted many 4BSD features, including:

  1. Job control
  2. Reliable signals
  3. Multiple file-access
  4. Filesystem interfaces

To be considered a Unix system, an operating system must meet the X/OPEN interface specifications, many of them shared by the POSIX.1 standard.

To circumvent the increasing cost of the AT&T licenses, vendors pushed Berkeley to release its TCP/IP networking code under separate licensing terms that did not require the AT&T license. This led to Berkeley's first software release, **Networking Release 1**, which was freely redistributable. This means that the licensee could release the code modified/unmodified in source or binary form with no accounting or royalties to Berkeley.

Keith Bostic led the movement to rewrite Unix utilities from scratch using their published descriptions in an effort to untangle BSD from Unix. The compensation developers received was their name next to the utility that they rewrote. Slowly but surely nearly all important utilities and libraries had been rewritten. Despite the fact that there were 6 remaining kernel files that could not be trivially rewritten, CSRG released the additional free utilities along with their TCP/IP implementation in their **Networking Release 2**.

Bill Jolitz rewrote the remaining 6 kernel files and released the fully compiled and bootable system called 386/BSD over anonymous FTP. However, his full time job prevented him from keeping up with bug fixes and enhancements, leading to the **NetBSD** fork. This distribution emphasizes portability and they target hardcore technical users.

**FreeBSD** resulted a few months later from a charter to support the PC architecture and target less a less advanced audience.

**OpenBSD** was created in 1995 to focus on improving the security of the BSD operating systems.

Three groups work on FreeBSD directly:

  1. 3000-4000 Developers
  2. 300-400 Committers
  3. 9 Core Team Members elected every two years by committers

Typical new committer to FreeBSD is in their mid to late 20s and has been programming Linux or other open-source projects for a decade.
