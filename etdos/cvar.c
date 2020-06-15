#include <string.h>
#include "common.h"
#include "cvar.h"

#define	MAX_CVARS	1024
cvar_t *cvar_vars;
cvar_t cvar_indexes[MAX_CVARS];
int cvar_numIndexes;
int cvar_modifiedFlags;

#define FILE_HASH_SIZE		256
static cvar_t* hashTable[FILE_HASH_SIZE];

static long generateHashValue( const char *fname )
{
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (fname[i] != '\0') {
		letter = tolower(fname[i]);
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash &= (FILE_HASH_SIZE-1);
	return hash;
}

static cvar_t *Cvar_FindVar( const char *var_name )
{
	cvar_t	*var;
	long hash;

	hash = generateHashValue(var_name);

	for (var=hashTable[hash] ; var ; var=var->hashNext) {
		if (!Q_stricmp(var_name, var->name)) {
			return var;
		}
	}

	return NULL;
}

float Cvar_VariableValue( const char *var_name )
{
	cvar_t	*var;

	var = Cvar_FindVar (var_name);
	if (!var)
		return 0;
	return var->value;
}

int Cvar_VariableIntegerValue( const char *var_name )
{
	cvar_t	*var;

	var = Cvar_FindVar (var_name);
	if (!var)
		return 0;
	return var->integer;
}

char *Cvar_VariableString( const char *var_name )
{
	cvar_t *var;

	var = Cvar_FindVar (var_name);
	if (!var)
		return "";
	return var->string;
}

void Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize )
{
	cvar_t *var;

	var = Cvar_FindVar (var_name);
	if (!var) {
		*buffer = 0;
	}
	else {
		strncpy( buffer, var->string, bufsize );
	}
}

char *Cvar_InfoString( int bit )
{
	static char	info[MAX_INFO_STRING];
	cvar_t	*var;

	info[0] = 0;

	for (var = cvar_vars ; var ; var = var->next) {
		if (var->flags & bit) {
			Info_SetValueForKey (info, var->name, var->string);
		}
	}
	return info;
}

cvar_t *Cvar_Get( const char *var_name, const char *var_value, int flags )
{
	cvar_t	*var;
	long	hash;

	if ( !var_name || ! var_value ) {
		emits("Cvar_Get: NULL parameter" );
		return NULL;
	}

	var = Cvar_FindVar (var_name);
	if ( var ) {
		if ( ( var->flags & CVAR_USER_CREATED ) && !( flags & CVAR_USER_CREATED ) && var_value[0] ) {
			var->flags &= ~CVAR_USER_CREATED;
			free( var->resetString );
			var->resetString = strdup( var_value );

			cvar_modifiedFlags |= flags;
		}

		var->flags |= flags;

		if ( !var->resetString[0] ) {
			free( var->resetString );
			var->resetString = strdup( var_value );
		} else if ( var_value[0] && strcmp( var->resetString, var_value ) ) {
			emitv( "Warning: cvar \"%s\" given initial values: \"%s\" and \"%s\"\n", var_name, var->resetString, var_value );
		}

		if ( var->latchedString ) {
			char *s;

			s = var->latchedString;
			var->latchedString = NULL;	// otherwise cvar_set2 would free it
			Cvar_Set2( var_name, s, qtrue );
			free( s );
		}

		return var;
	}

	if ( cvar_numIndexes >= MAX_CVARS ) {
		emits("MAX_CVARS" );
		return NULL;
	}

	var = &cvar_indexes[cvar_numIndexes];
	cvar_numIndexes++;
	var->name = strdup (var_name);
	var->string = strdup (var_value);
	var->modified = qtrue;
	var->modificationCount = 1;
	var->value = atof (var->string);
	var->integer = atoi(var->string);
	var->resetString = strdup( var_value );

	var->next = cvar_vars;
	cvar_vars = var;

	var->flags = flags;

	hash = generateHashValue(var_name);
	var->hashNext = hashTable[hash];
	hashTable[hash] = var;

	return var;
}

cvar_t *Cvar_Set2( const char *var_name, const char *value, qboolean force )
{
	cvar_t	*var;

	var = Cvar_FindVar (var_name);
	if (!var) {
		if ( !value ) {
			return NULL;
		}

		if ( !force ) {
			return Cvar_Get( var_name, value, CVAR_USER_CREATED );
		} else {
			return Cvar_Get (var_name, value, 0);
		}
	}

	if (!value ) {
		value = var->resetString;
	}

	if((var->flags & CVAR_LATCH) && var->latchedString) {
		if(!strcmp(value,var->latchedString))
			return var;
	}
	else if (!strcmp(value,var->string)) {
		return var;
	}

	cvar_modifiedFlags |= var->flags;

	if (!force)
	{
		if (var->flags & CVAR_ROM)
		{
			emitv ("%s is read only.\n", var_name);
			return var;
		}

		if (var->flags & CVAR_INIT)
		{
			emitv ("%s is write protected.\n", var_name);
			return var;
		}

		if (var->flags & CVAR_LATCH)
		{
			if (var->latchedString)
			{
				if (strcmp(value, var->latchedString) == 0)
					return var;
				free (var->latchedString);
			}
			else
			{
				if (strcmp(value, var->string) == 0)
					return var;
			}

			emitv ("%s will be changed upon restarting.\n", var_name);
			var->latchedString = strdup(value);
			var->modified = qtrue;
			var->modificationCount++;
			return var;
		}
	}
	else
	{
		if (var->latchedString)
		{
			free (var->latchedString);
			var->latchedString = NULL;
		}
	}

	if (!strcmp(value, var->string))
		return var;		// not changed

	var->modified = qtrue;
	var->modificationCount++;

	free (var->string);

	var->string = strdup(value);
	var->value = atof (var->string);
	var->integer = atoi (var->string);

	return var;
}
