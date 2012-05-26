/* ddate.c .. converts boring normal dates to fun Discordian Date -><-
   written  the 65th day of The Aftermath in the Year of Our Lady of 
   Discord 3157 by Druel the Chaotic aka Jeremy Johnson aka
   mpython@gnu.ai.mit.edu  

   and I'm not responsible if this program messes anything up (except your 
   mind, I'm responsible for that)

   Modifications for Unix by Lee Harvey Oswald Smith, K.S.C.
   Five tons of flax.
   
   Modifications for happyblog by MrLoom, K.S.C.
   Dash it! Soft tabs with a tab size of 2, really?
   -- Order and Disorder.
*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>


struct disc_time
{int season; /* 0-4 */
 int day; /* 0-72 */
 int yday; /* 0-365 */
 int year; /* 3066- */
};


static char *ending(int num)
{  
 int temp;
 char *funkychickens;
 
 funkychickens=(char *)malloc(sizeof(char)*3);
 
  temp=num%10; /* get 0-9 */  
  switch (temp)
  { case 1:
      strcpy(funkychickens,"st");
      break;
    case 2:
      strcpy(funkychickens,"nd");
      break;
    case 3:
      strcpy(funkychickens,"rd");
      break;
    default:
      strcpy(funkychickens,"th");
    }
 return funkychickens;
}

static struct disc_time convert(int nday, int nyear)
{  struct disc_time funkychickens;
   
   funkychickens.year = nyear+3066;
   funkychickens.day=nday;
   funkychickens.season=0;
   if ((funkychickens.year%4)==2)
     {if (funkychickens.day==59)
       funkychickens.day=-1;
     else if (funkychickens.day >59)
       funkychickens.day-=1;
    }
   funkychickens.yday=funkychickens.day;
   while (funkychickens.day>=73)
     { funkychickens.season++;
       funkychickens.day-=73;
     }
   return funkychickens;
  
 }

void ddate(char *buffer, size_t maxsize, struct tm *eris)
{ char *days[5] = { "Sweetmorn",
                    "Boomtime",
                    "Pungenday",
                    "Prickle-Prickle",
                    "Setting Orange"
  };
  char *seasons[5] = {"Chaos",
                      "Discord",
                      "Confusion",
                      "Bureaucracy",
                      "The Aftermath"
  };
  struct disc_time tick;
  tick=convert(eris->tm_yday, eris->tm_year);
  if (tick.day==-1) snprintf(buffer, maxsize, "St. Tib's Day in the YOLD %d",tick.year);
  else
    { tick.day++;
      snprintf(buffer, maxsize, "%s, %d%s day of %s - YOLD %d", days[tick.yday%5], tick.day, ending(tick.day),seasons[tick.season], tick.year);
    }
}

