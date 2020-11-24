# Walthrough of Kernel Modules and Dynamic Linkers

## Overview

1. _module.c_: Loading Dynamic Kernel Linker

## _module.c_: Loading the Dynamic Kernel Linker

### Important Structures

```c
/*
 * Preloaded file metadata header.
 *
 * Metadata are allocated on our heap, and copied into kernel space
 * before executing the kernel.
 */
struct file_metadata 
{
    size_t			md_size;
    u_int16_t			md_type;
    struct file_metadata	*md_next;
    char			md_data[1];	/* data are immediately appended */
};

struct kernel_module
{
    char			*m_name;	/* module name */
    int				m_version;	/* module version */
/*    char			*m_args;*/	/* arguments for the module */
    struct preloaded_file	*m_fp;
    struct kernel_module	*m_next;
};

/*
 * Preloaded file information. Depending on type, file can contain
 * additional units called 'modules'.
 *
 * At least one file (the kernel) must be loaded in order to boot.
 * The kernel is always loaded first.
 *
 * String fields (m_name, m_type) should be dynamically allocated.
 */
struct preloaded_file
{
    char			*f_name;	/* file name */
    char			*f_type;	/* verbose file type, eg 'ELF kernel', 'pnptable', etc. */
    char			*f_args;	/* arguments for the file */
    struct file_metadata	*f_metadata;	/* metadata that will be placed in the module directory */
    int				f_loader;	/* index of the loader that read the file */
    vm_offset_t			f_addr;		/* load address */
    size_t			f_size;			/* file size */
    struct kernel_module	*f_modules;	/* list of modules if any */
    struct preloaded_file	*f_next;	/* next file */
};
```

### Loading Code

```c
/*
 * Load specified KLD. If path is omitted, then try to locate it via
 * search path.
 */
int
mod_loadkld(const char *kldname, int argc, char *argv[])
{
    struct preloaded_file	*fp, *last_file;
    int				err;
    char			*filename;

    /*
     * Get fully qualified KLD name
     */
    filename = file_search(kldname, kld_ext_list);
    if (filename == NULL) {
	sprintf(command_errbuf, "can't find '%s'", kldname);
	return (ENOENT);
    }
    /* 
     * Check if KLD already loaded
     */
    fp = file_findfile(filename, NULL);
    if (fp) {
	sprintf(command_errbuf, "warning: KLD '%s' already loaded", filename);
	free(filename);
	return (0);
    }
    for (last_file = preloaded_files; 
	 last_file != NULL && last_file->f_next != NULL;
	 last_file = last_file->f_next)
	;

    do {
	err = file_load(filename, loadaddr, &fp);
	if (err)
	    break;
	fp->f_args = unargv(argc, argv);
	loadaddr = fp->f_addr + fp->f_size;
	file_insert_tail(fp);		/* Add to the list of loaded files */
	if (file_load_dependencies(fp) != 0) {
	    err = ENOENT;
	    last_file->f_next = NULL;
	    loadaddr = last_file->f_addr + last_file->f_size;
	    fp = NULL;
	    break;
	}
    } while(0);
    if (err == EFTYPE)
	sprintf(command_errbuf, "don't know how to load module '%s'", filename);
    if (err && fp)
	file_discard(fp);
    free(filename);
    return (err);
}
```
