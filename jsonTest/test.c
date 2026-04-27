#include <stdio.h>
#include "../src/cJSON.c"

int main() {
	// open the JSON file
	FILE *fp = fopen("data.json", "r");
	if (fp == NULL) {
		printf("Error: Unable to open the file.\n");
		return 1;
	}

	// read the file contents into a string
	char buffer[1024];
	int len = fread(buffer, 1, sizeof(buffer), fp);
	fclose(fp);

	// parse the JSON data
	cJSON *json = cJSON_Parse(buffer);
	if (json == NULL) {
		const char *error_ptr = cJSON_GetErrorPtr();
		if (error_ptr != NULL) {
			printf("Error: %s\n", error_ptr);
		}
		cJSON_Delete(json);
		return 1;
	}

	// modify the JSON data
	cJSON_ReplaceItemInObjectCaseSensitive(json, "name", cJSON_CreateString("Jane Doe"));
	cJSON_AddNumberToObject(json, "age", 32);
	cJSON_AddStringToObject(json, "phone", "555-555-5555");

	// convert the cJSON object to a JSON string
	char *json_str = cJSON_Print(json);

	// write the JSON string to the file
	fp = fopen("data.json", "w");
	if (fp == NULL) {
		printf("Error: Unable to open the file.\n");
		return 1;
	}
	printf("%s\n", json_str);
	fputs(json_str, fp);
	fclose(fp);

	// free the JSON string and cJSON object
	cJSON_free(json_str);
	cJSON_Delete(json);

	return 0;
}