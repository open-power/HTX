/* IBM_PROLOG_BEGIN_TAG */
/* 
 * Copyright 2003,2016 IBM International Business Machines Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 		 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* IBM_PROLOG_END_TAG */

/*****************************************************************************
*   COMPONENT_NAME: exer_tape
* 
*   MODULE NAME: posix.c
*****************************************************************************/

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

/*
 * A bug-free version of the string calls
 * Just to make sure that the calls work properly and at the same time are
 * POSIX compliant.
 */
size_t	htx_strlen(const char *string)
{
#ifdef	__HTX_LINUX__
	if(string == (const char *) NULL)  {
		return 0;
	}
#endif

	return	strlen(string);
}

char	*htx_strcpy(char *dest, const char *src)
{
#ifdef	__HTX_LINUX__
	if(dest == (const char *) NULL)  {
		return NULL;
	}

	else if(src == (const char *) NULL)  {
		return dest;
	}
#endif

	return	strcpy(dest, src);
}

char	*htx_strncpy(char *dest, const char *src, size_t n)
{
#ifdef	__HTX_LINUX__
	if(dest == (const char *) NULL)  {
		return NULL;
	}

	else if(src == (const char *) NULL)  {
		return dest;
	}
#endif

	return	strncpy(dest, src, n);
}

char	*htx_strcat(char *dest, const char *src)
{
#ifdef	__HTX_LINUX__
	if(dest == (const char *) NULL)  {
		return NULL;
	}

	else if(src == (const char *) NULL)  {
		return dest;
	}
#endif

	return	strcat(dest, src);
}

char	*htx_strchr(const char *s, int c)
{
#ifdef	__HTX_LINUX__
	if(s == (const char *) NULL)  {
		return NULL;
	}
#endif

	return	strchr(s, c);
}

int	htx_strcmp(const char *s1, const char *s2)
{
#ifdef	__HTX_LINUX__
	if(s1 == (const char *) NULL)  {
		return -1;
	}

	else if(s2 == (const char *) NULL)  {
		return 1;
	}
#endif

	return	strcmp(s1, s2);
}

int	htx_strncmp(const char *s1, const char *s2, size_t n)
{
#ifdef	__HTX_LINUX__
	if(s1 == (const char *) NULL)  {
		return -1;
	}

	else if(s2 == (const char *) NULL)  {
		return 1;
	}
#endif

	return	strncmp(s1, s2, n);
}

size_t	htx_strspn(const char *s, const char *accept)
{
#ifdef	__HTX_LINUX__
	if((s == (const char *) NULL) || (accept == (const char *) NULL))   {
		return 0;
	}
#endif

	return	strspn(s, accept);
}

size_t	htx_strcspn(const char *s, const char *reject)
{
#ifdef	__HTX_LINUX__
	if((s == (const char *) NULL) || (reject == (const char *) NULL))   {
		return 0;
	}
#endif

	return	strcspn(s, reject);
}


