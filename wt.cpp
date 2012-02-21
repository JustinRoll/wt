#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include <curl/curl.h>

void * mainloop(void *);
void * checkloop(void *);

struct tw;

struct tw
{
	struct tw* prev;
	struct tw* next;
	long min;
	time_t sched_time;
	char url[BUFSIZ];
};

struct tw* tw_head = NULL;
long tw_size = 0;

struct tw* tw_at(long loc)
{
	struct tw* tw_cur;

	if (loc < 0)
		return NULL;
	if (loc > tw_size-1)
		return NULL;

	tw_cur = tw_head;
	for( ; loc > 0; loc--)
	{
		if (tw_cur == NULL)
			return NULL;
		tw_cur = tw_cur->next;
	}

	return tw_cur;

}


int main(int argc, char ** argv)
{
	pthread_t t1;

	if (argc == 1)
	{
		// proper run
		pthread_create(&t1, NULL, checkloop, NULL);
		mainloop(NULL);
		return 0;

	}

	return 0;

}


void * mainloop(void *)
{
	char c;
	char str[BUFSIZ];
	char * str_loc;
	char * curloc;
	char * minloc;
	//struct tw* tw_head = NULL;
	struct tw* tw_cur = NULL;
	struct tw* tw_new = NULL;
	long count;
	long val, val2;
	char timestr[BUFSIZ];
	

	while (true)
	{
		gets(str);

		//process input
		//	possible inputs:
		//		list
		//		delete
		//		add
		//		edit
		//		settings
		curloc = strtok_r(str, " ", &str_loc);
		if (strlen(str) > 1)
		{
			// invalid input
			continue;
		}

		c = str[0];

		if (c == '?')
		{
			printf("Usage:\n"
				"l\t\t List active URLs\n"
				"a URL\t\t Add URL to watch\n"
				"d NUM\t\t Delete element NUM from list\n"
				"e NUM\t\t Enter edit mode for element NUM\n"
				"s\t\t Enter settings mode\n");

		}
		else if (c == 'l')
		{
			tw_cur = tw_head;

			count = 0;
			while (tw_cur != NULL)
			{
				strftime(timestr, BUFSIZ, "%c", localtime(&tw_cur->sched_time));
				printf("[%d] %x %s %s\n", count, tw_cur, timestr, tw_cur->url);

				tw_cur = tw_cur->next;
				count += 1;
			}
		}
		else if (c == 'd')
		{
			curloc = strtok_r(NULL, " ", &str_loc);
			if (curloc == 0)
			{
				printf("Usage: d NUMBER\n");
				continue;
			}

			val = atoi(curloc);

			// delete element 'val'
			tw_cur = tw_at(val);

			if (tw_cur == NULL)
				continue;

			// patch pointers
			if (tw_cur->prev == NULL)
			{
				if (tw_cur->next != NULL)
				{
					tw_cur->next->prev = NULL;
					tw_head = tw_cur->next;
				}
				else
				{
					tw_head = NULL;
				}

			}
			else
			{
				if (tw_cur->next != NULL)
				{
					tw_cur->prev->next = tw_cur->next;
					tw_cur->next->prev = tw_cur->prev;
				}
				else
				{
					tw_cur->prev->next = NULL;
				}
			}
			free(tw_cur);
			tw_size -= 1;

		}
		else if (c == 'a')
		{
			// make sure format is:
			//	a URL
			curloc = strtok_r(NULL, " ", &str_loc);
			if (curloc == 0)
			{
				printf("Usage: a URL\n");
				continue;
			}


			tw_size += 1;

			if (tw_head == NULL)
			{
				tw_head = (struct tw*)calloc(1, sizeof(struct tw));
				tw_head->min = 1;
				time( &tw_head->sched_time );
				tw_head->sched_time += tw_head->min * 60;
				strncpy(tw_head->url, curloc, BUFSIZ);

			}
			else
			{
				// traverse the list to find when .next = NULL
				tw_cur = tw_head;

				while (tw_cur->next != NULL)
				{
					tw_cur = tw_cur->next;
				}

				// found insertion point
				tw_new = (struct tw*)calloc(1, sizeof(struct tw));
				tw_new->prev = tw_cur;
				tw_cur->next = tw_new;

				tw_new->min = 1;
				time( &tw_new->sched_time );
				tw_new->sched_time += tw_new->min * 60;
				strncpy(tw_new->url, curloc, BUFSIZ);

			}
		}
		else if (c == 'e')
		{
			if (tw_head == NULL)
				continue;	
		
			curloc = strtok_r(NULL, " ", &str_loc);
			minloc = strtok_r(NULL, " ", &str_loc);
			if (curloc == 0 || minloc == 0)
			{
				printf("Usage: e NUM MIN\n");
				continue;
			}

			val = atoi(curloc);
			val2 = atoi(minloc);

			count = 0;
			tw_cur = tw_head;

			if (val < 0)
				continue;

			while (count < val)
			{
				if (tw_cur == NULL)
				{
					printf("Cannot find element %d\n", val);
					goto leaveloop;
				}

				tw_cur = tw_cur->next;
				count += 1;
			}

			if (val2 > 1)
				tw_cur->min = val2;
			else
				tw_cur->min = 1;
leaveloop:
			continue;

		}
		else if (c == 's')
		{
			printf("SETTINGS\n");
		}

	}



}

void * checkloop(void *)
{
	int aflag = 0;
	int index = 0;
	char c;
	char url[BUFSIZ];
	char outfilestr[BUFSIZ];
	CURL *curl;
	CURLcode res;
	FILE *fp;
	pthread_t t2;
	time_t s_time;
	time_t c_time;
	struct tw* tw_cur = NULL;
	int count;



		time (&s_time);

		while (true)
		{
			time (&c_time);
			if (c_time >= s_time)
			{
				tw_cur = tw_head;

				count = 0;
				while (tw_cur != NULL)
				{
					if (c_time >= tw_cur->sched_time)
					{
						// do update
						//printf("Updating element %d\n", count);
						strncpy(url, tw_cur->url, BUFSIZ);
						url[BUFSIZ-1] = 0;
						curl = curl_easy_init();
						if (curl)
						{
							snprintf(outfilestr, BUFSIZ, "%d.html", count);
							fp = fopen(outfilestr, "wb");
							curl_easy_setopt(curl, CURLOPT_URL, url);
							curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
							curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
							res = curl_easy_perform(curl);
		
							curl_easy_cleanup(curl);
							fclose(fp);
						}

						tw_cur->sched_time += tw_cur->min * 60;

						//printf("Update complete\n");

					}

					count += 1;
					tw_cur = tw_cur->next;
				}
				s_time += 1;
			}
		}
}
