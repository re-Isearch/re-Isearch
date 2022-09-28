#include <stdio.h>
#include <string.h>

int main()
{

// city,lat,lng,country,iso2,admin_name,capital,population,population_proper
 char tmp[256];
 char *tcp;
 char *city = tmp;
 char *lat;
 char *lng;
 char *country;
 char sep = ',';

 while ((tcp = gets(tmp)) != NULL) {
   if ((lat  = strchr(city, sep)) == NULL) break; 
   *lat++ = '\0';
   if ((lng  = strchr(lat, sep)) == NULL) break;
   *lng++ = '\0';
   if ((tcp  = strchr(lng, sep)) == NULL) break;
   *tcp = '\0';


   printf("{\"%s\", %s, %s}, \n", city, lat, lng);

 }


}
