# Walthrough of _environment API_

### Overview

1. _setenv_
2. _\_getenv\_dynamic_

### _setenv_

```c
/*
 * Set an environment variable by name.
 */
int
setenv(const char *name, const char *value)
{
	char *buf, *cp, *oldenv;
	int namelen, vallen, i;

	if (!dynamic_kenv)
		panic("%s: called before SI_SUB_KMEM", __func__);

	namelen = strlen(name) + 1;
	if (namelen > KENV_MNAMELEN)
		return (-1);
	vallen = strlen(value) + 1;
	if (vallen > KENV_MVALLEN)
		return (-1);
	buf = malloc(namelen + vallen, M_KENV, M_WAITOK);
	sprintf(buf, "%s=%s", name, value);

	sx_xlock(&kenv_lock);
	cp = _getenv_dynamic(name, &i);
	if (cp != NULL) {		// env variable previously set
		oldenv = kenvp[i];
		kenvp[i] = buf;
		sx_xunlock(&kenv_lock);
		free(oldenv, M_KENV);
	} else {				// env variable never set
		/* We add the option if it wasn't found */
		for (i = 0; (cp = kenvp[i]) != NULL; i++);	// Find first null entry
		kenvp[i] = buf;
		kenvp[i + 1] = NULL;
		sx_xunlock(&kenv_lock);
	}
	return (0);
}
```

## _\_getenv\_dynamic_

```c
/*
 * Internal functions for string lookup.
 */
static char *
_getenv_dynamic(const char *name, int *idx)
{
	char *cp;
	int len, i;

	sx_assert(&kenv_lock, SX_LOCKED);
	len = strlen(name);
	for (cp = kenvp[0], i = 0; cp != NULL; cp = kenvp[++i]) {
		if ((cp[len] == '=') &&
		    (strncmp(cp, name, len) == 0)) {
			if (idx != NULL)
				*idx = i;
			return (cp + len + 1);
		}
	}
	return (NULL);
}
```
