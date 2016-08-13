Magma: Graphics for Magenta
===========================

Magma is a framework for graphics drivers on the Magenta kernel. Magma drivers are logically divided into a 'System Driver' which runs as a userspace Magenta driver service, and an 'Application Driver' which runs in the application's address space (this mirrors the architecture of the 'Kernel Mode Driver' and 'User Mode Driver' in traditional graphics stack for monolithic kernels, but here both components run in userspace).

Magma itself is the body of software that sits between the Application Driver and the System Driver and facilitates communication between the two over Magenta IPC, and provides core buffer sharing logic which underlies the system compositing mechanism. 

Both the Application Driver and the System Driver interface with the Magma framework through stable, versioned, ABI's in order to allow updating the core graphics system and IHV drivers independently of one another.
