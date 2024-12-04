#include "UserData.h"
#include "exercises.h"
#include "workout_program.h"
#include "P1.h"

typedef long equipment_avalibility;

enum equipment_e {
    DUMBBELLS = 0x1,
    BARBELL = 0x2,
    MACHINE = 0x4,
    CABLE = 0x8,
    BODYWEIGHT = 0x10,
};


/**@brief Struct to track changes in workout performance*/
typedef struct workout_result_t {
    int repChange;        // Change in number of reps
    double weightChange;  // Change in weight used
} workout_result_t;

char userprofile_path[MAX_LENGTH+15];

int userfile_workout_counter = 0;

user_context_t local_userContext = {
    .userFILE = NULL,
    .userPrefs = NULL,
    .contextExists = USERDATA_CONTEXT_INVALID
};


/**@brief Handles the signup process for new users
 * @param map Hashmap containing existing usernames
 * @param accountsPath Path to the accounts file
 * @param accountsFILE File pointer to the accounts database
 * @param input Buffer to store user input
 * @return USERDATA_SUCCESS on success, USERDATA_FAILURE on failure */
int handle_signup(HashMap_t* map, char* accountsPath, FILE* accountsFILE, char* input) {
    
    // Read username from user
    printf("\nEnter your desired username (Max %d characters): ", MAX_LENGTH - 1);
    if (scanf("%s", input) != 1) {
        printf("Error reading username\n");
        return USERDATA_FAILURE;
    }

    
    // Validate username length
    if (strlen(input) >= MAX_LENGTH) {
        printf("\nUsername too long! Maximum %d characters.\n", MAX_LENGTH - 1);
        return USERDATA_FAILURE;
    }


    // Convert username to lowercase
    for (int n = 0; input[n]; n++) {
        input[n] = tolower(input[n]);
    }


    // Check if username already exists
    if (verify_user_existence(input, map)) {
        printf("\nUsername already exists!\n");
        return USERDATA_FAILURE;
    }


    // Validate file pointer
    if (!accountsFILE) {
        printf("\nA valid file has not been passed\n");
        return USERDATA_FAILURE;
    }


    // Create user's data file within the current context
    local_userContext.userFILE = create_new_user(accountsFILE, input, map);


    // Revalidate creation (the function should return error if else)
    if (!local_userContext.userFILE) {
		printf("\Could not validate user file!\n");
		return USERDATA_FAILURE;
	}


    // Initialize user preferences and exercise data
    int _days = 0; int _time = 0;
    user_preferences_prompt(&_days, &_time);
    fill_user_data(&local_userContext, _days, _time);


    // Add new user to hashmap
    set(map, input, map->size * MAX_LENGTH);


    // Read user preferences, this also acts as a validation step
    user_file_header_prefs* prefs = read_user_preferences(local_userContext.userFILE);
    if(!prefs) {
		printf("Error reading user preferences\n");
		return USERDATA_FAILURE;
	}


    // Generate a new workout program based on user preferences, and update user data
    update_user_workout_data(
        generate_workout_program(*prefs));


    // Reopen accounts file in read mode and flush the context
    fclose(accountsFILE);
    fclose(local_userContext.userFILE);
    accountsFILE = fopen(accountsPath, "r");

    
    // Free resources
    free(prefs);


    return USERDATA_SUCCESS;
}


/**@brief Handles the login process for existing users
 * @param map Hashmap containing existing usernames
 * @param userFILE Pointer to the user's data file
 * @param input Buffer to store user input
 * @return USERDATA_SUCCESS on success, USERDATA_FAILURE on failure */
int handle_login(HashMap_t* map, FILE* userFILE, char* input) {
    
    // Read username from user
    printf("\nEnter your username: ");
    if (scanf(" %s", input) != 1) {
        printf("Error reading username\n");
        return USERDATA_FAILURE;
    }


    // Convert username to lowercase
    for (int n = 0; input[n]; n++) {
        input[n] = tolower(input[n]);
    }


    // Check if username exists
    if (!verify_user_existence(input, map)) {
        printf("\nUsername does not exist!\n");
        return USERDATA_FAILURE;
    }

    
    // Construct path to user's data file
    printf("\nLogged in as %s!\n", input);
    char filepath[MAX_LENGTH + 15];
    snprintf(filepath, sizeof(filepath), "%s/%s.dat", USER_FILES_DIR, input);
    userFILE = fopen(filepath, "rb+");


    // Validate file pointer
    if (!userFILE) {
        printf("Error opening user file\n");
        return USERDATA_FAILURE;
    }


    // Read user data into exercises_c
    parse_user_data_into_array(exercises_c);


    return USERDATA_SUCCESS;  // Successfully logged in
}


/**@brief Establishes a context for user data
 * @return USERDATA_SUCCESS on success, USERDATA_FAILURE on failure
 * @note This function is the entry point for the UserData module
 * @note This function should be called before any other UserData functions
 * @note This function should be called only once per program execution*/
int establish_userdata_context() {
    
    // Ensure user directory exists before opening files
    if (!ensure_user_directory()) {
        printf("Error: Could not create user directory\n");
        return USERDATA_FAILURE;
    }


    // Construct path to accounts file
    char accountsPath[MAX_LENGTH + 15];
    snprintf(accountsPath, sizeof(accountsPath), "%s/%s", USER_FILES_DIR, USER_ACCOUNTS_FILE);


    // Open accounts file in binary read/write mode
    FILE* accountsFILE = fopen(accountsPath, "rb+");
    if (!accountsFILE) {
        printf("Error: Could not open accounts file\n");
        return USERDATA_FAILURE;
    }


    // Initialize username hashmap
    HashMap_t* map = createHashMap();
    if (!map) {
        printf("Error: Could not create hashmap\n");
        fclose(accountsFILE);
        return USERDATA_FAILURE;
    }


    // Load existing accounts into hashmap
    int m = startHashMap(accountsFILE, map);


    char input[MAX_LENGTH];


    // Main program loop
    int running = 1;
    while (running) {
        login_screen:
        printf("\nLogin or signup? (or 'exit' to quit)\n");
        printf("Type 'login' if you already have an account.\n");
        printf("Type 'signup' if you want to create a new account.\n");


        // Read user input
        if (scanf("%s", input) != 1) {
            printf("Error reading input\n");
            continue;
        }


        // Convert input to lowercase
        for(int n = 0; input[n]; n++) {
            input[n] = tolower(input[n]);
        }


        // Handle signup process
        if (strcmp(input, "signup") == 0) {
            if (!handle_signup(map, accountsPath, accountsFILE, input)) {
                goto login_screen;
            }
        }


        // Handle login process
        else if (strcmp(input, "login") == 0) {
            if (handle_login(map, local_userContext.userFILE, input)) {
                goto login_screen;
            }
            else {
                break;
            }
        }


        // Exit program
        else if (strcmp(input, "exit") == 0) {
            exit(0);
        }

        // Invalid input
        else {
            printf("\nInvalid input!\n");
            goto login_screen;
        }
    }


    // Cleanup resources
    if (accountsFILE) fclose(accountsFILE);
    freeHashMap(map);


    // Setup context
    local_userContext.contextExists = USERDATA_CONTEXT_VALID;
    local_userContext.userPrefs = read_user_preferences(local_userContext.userFILE);

    return USERDATA_SUCCESS; // Context successfully established
}


/**@brief Validates a user_context
 * @param user_context Pointer to the user_context_t to be validated
 * @return USERDATA_SUCCESS if valid, USERDATA_FAILURE if invalid */
int validate_user_context(user_context_t* user_context) {
    if ((user_context->userFILE == NULL) || (!user_context->contextExists) || (user_context->userPrefs == NULL)) {
        return USERDATA_FAILURE;
    }
    return USERDATA_SUCCESS;
}


/**@brief Verifies whether a username exists in the hashmap
 * @param usernameInput The username to verify
 * @param map The hashmap containing valid usernames
 * @return int 1 if username exists, 0 if it doesn't exist */
int verify_user_existence(char* usernameInput, HashMap_t* map) {
    int value = get(map, usernameInput);
    return (value != -1);
}


/**@brief Reads usernames from a file and stores them in a hashmap
 * @param accountsFILE The open file containing the user accounts
 * @param map The hashmap to store the usernames and their file positions
 * @return int The number of usernames loaded into the hashmap */
int startHashMap(FILE* accountsFILE, HashMap_t* map) {
    char tempUsername[MAX_LENGTH];
    int m = 0;

    // Read line by line
    while (fgets(tempUsername, MAX_LENGTH, accountsFILE)) {
        // Remove newline character if present
        size_t len = strlen(tempUsername);
        if (len > 0 && (tempUsername[len-1] == '\n' || tempUsername[len-1] == '\r')) {
            tempUsername[len-1] = '\0';
        }
        // Remove second character of CRLF if present
        len = strlen(tempUsername);
        if (len > 0 && (tempUsername[len-1] == '\n' || tempUsername[len-1] == '\r')) {
            tempUsername[len-1] = '\0';
        }

        printf("Read username: '%s'\n", tempUsername);
        set(map, tempUsername, m * MAX_LENGTH);
        m++;
    }
    printf("\nLoaded %d usernames into the hashmap\n", m);
    return m;
}


/**@brief Creates necessary directories for user files if they don't exist
 * @return int Returns 1 on success, 0 on failure */
static int ensure_user_directory() {
    struct stat st = {0};
    if (stat(USER_FILES_DIR, &st) == -1) {
        #ifdef _WIN32
        return mkdir(USER_FILES_DIR) == 0;
        #else
        return mkdir(USER_FILES_DIR, 0700) == 0;
        #endif
    }
    return 1;
}


/**@brief Creates a new user and adds them to the system
 * @param accountsFILE File pointer to the accounts database
 * @param username Username to create (max MAX_LENGTH characters)
 * @param map Hashmap containing existing usernames
 * @return FILE* Pointer to the newly created user file, NULL if creation fails */
FILE* create_new_user(FILE* accountsFILE, char username[MAX_LENGTH], HashMap_t* map) {
    
    // Ensure user directory exists
    if (!ensure_user_directory()) {
        printf("Error: Could not create user directory\n");
        return NULL;
    }


    // Discover the position of the new username in the accounts file
    long position = ftell(accountsFILE);
    
    
    // Write the new username to the accounts file
    fseek(accountsFILE, 0, SEEK_END);
    fprintf(accountsFILE, "%s\n", username);

    
    // Flush the accounts file to disk
    fflush(accountsFILE);

    
    // Add the user to the hashmap
    set(map, username, position);


    // Construct path to user's data file and generate userFILE
    char filepath[MAX_LENGTH + 15];
    snprintf(filepath, sizeof(filepath), "%s/%s.dat", USER_FILES_DIR, username);
    FILE* userFILE = fopen(filepath, "wb+");


    return userFILE; // Return the newly created user file
}


/**@brief prompts user preferences for workout program generation
 * @param userFILE Pointer to the user's data file
 * @param days Pointer to the variable to store the number of days
 * @param time Pointer to the variable to store the time
 * @note This function is called during the signup process */
void user_preferences_prompt(int* days, int* time) {

    printf("\nWelcome! Let's set up your preferences.\n");

    // Get days input with input validation
    do {
        printf("Enter preferred number of days you would like to workout(1-7): ");
        if (scanf("%d", days) != 1 || *days < 1 || *days > 7) {
            printf("\nInvalid input! Please enter a number between 1 and 7.\n");
            while (getchar() != '\n'); // Clear input buffer for retry
            continue;
        }
        break;
    } while (1);

    // Get time input with input validation
    do {
        printf("Enter preferred time in minutes (15-120): ");
        if (scanf("%d", time) != 1 || *time < 15 || *time > 120) {
            printf("\nInvalid input! Please enter a number between 15 and 120.\n");
            while (getchar() != '\n'); // Clear input buffer for retry
            continue;
        }
        break;
    } while (1);

    printf("\nPreferences saved successfully!\n");
}


/**@brief Initializes exercise data for all exercises in the exercises_c array
 * @param userFILE Pointer to the file where exercise data will be written
 * @note Writes weight (double) and reps (int) for each exercise
 * @note Skips past user preferences section before writing exercise data */
void fill_user_data(user_context_t* userContext, int days, int time) {

    // Validate userContext
    if (!validate_user_context(userContext)) {
		printf("Error: Invalid user context\n");
		return;
	}

    // Default workout
    workout_days_t* workout = malloc(sizeof(workout_days_t) * days);
    for (int i = 0; i < days; i++) {
        workout[i] = (workout_days_t){
            .compound = {0},
            .secondary = {0},
            .tertiary = {0},
        };
    }


    // Write user preferences to file
    user_file_header_prefs user_file_header_prefs = {
        .prefered_days = days,
        .perfered_time = time,
        .workout_counter = 0,
    };

    user_file_header_data user_file_header_data = {
		.workout = workout
	};

    for (int i = 0; i < AMOUNT_EXERCISES; i++) {
        // Initialize exercise data with default values
        user_file_header_data.exercise_data[i] = (user_file_exercise_data){
            .weight = 10.0,
            .reps = 7,
        };
    }


    // Write user preferences explicitly to file with error checking
    if (fwrite(&user_file_header_prefs.prefered_days, sizeof(int), 1, userContext->userFILE) != 1) {
		printf("\nError writing preferences to file!\n");
	}

    if (fwrite(&user_file_header_prefs.perfered_time, sizeof(int), 1, userContext->userFILE) != 1) {
        printf("\nError writing preferences to file!\n");
    }

    if (fwrite(&user_file_header_prefs.workout_counter, sizeof(int), 1, userContext->userFILE) != 1) {
        printf("\nError writing preferences to file!\n");
    }


    // Write workout days to file explicitly with error 
    if (fwrite(workout, sizeof(workout_days_t), days, userContext->userFILE) != days) {
		printf("\nError writing workout days to file!\n");
	}


    // Write exercise data to file explicitly with error checking
    for(int i = 0; i < AMOUNT_EXERCISES; i++) {
		if (fwrite(&user_file_header_data.exercise_data[i].weight, sizeof(double), 1, userContext->userFILE) != 1) {
			printf("\nError writing exercise data to file!\n");
		}

		if (fwrite(&user_file_header_data.exercise_data[i].reps, sizeof(int), 1, userContext->userFILE) != 1) {
			printf("\nError writing exercise data to file!\n");
		}
	}


    // Flush the file to ensure data is written to disk
    fflush(userContext->userFILE);
    printf("\nUser Exercise Data saved successfully!\n");
    

    // Free resources
    free(workout);
}


/**@brief Parses user data from the user's file and assigns it to exercise structures
 * @param exercises Pointer to the exercise array to update with user data */
void parse_user_data_into_array(exercise_t* exercises) {
    for (int i = 0; i < AMOUNT_EXERCISES; i++) { 
        
        // Assign user data to exercise structure
        exercises[i].user_exercise_data = (user_exercise_data_t*) malloc(sizeof(user_exercise_data_t));
        if(exercises[i].user_exercise_data == NULL) {
			printf("Error: Could not allocate memory for user exercise data\n");
			return;
		}
        

        // Read exercise data from user file
        read_single_exercise_data(&local_userContext, i, exercises[i].user_exercise_data);
        
        
        // Assign exercise to user data for double link
        exercises[i].user_exercise_data->exercise = &(exercises_c[i]);
    }
}


/**@brief Reads exercise data for a single exercise from the user's file
 * @param userFILE Pointer to the file containing user data
 * @param exercise_index Index of the exercise to read data for
 * @param data Pointer to the user_exercise_data_t structure to store the data
 * @note This function reads weight and reps for a single exercise */
void read_single_exercise_data(user_context_t* user_context, int exercise_index, user_exercise_data_t* data) {

    const size_t SKIP_PREFS = sizeof(user_file_header_prefs);
    const size_t RECORD_SIZE = sizeof(double)+sizeof(int);

    // Validate user_context
    if (!validate_user_context(user_context)) {
        printf("Error: Invalid user context\n");
        return;
    }


    // Calculate file position for desired exercise
    long position = SKIP_PREFS + user_context->userPrefs->prefered_days * sizeof(workout_days_t) + exercise_index * RECORD_SIZE;


    // Seek to exercise position
    if (fseek(user_context->userFILE, position, SEEK_SET) != 0) {
        printf("Error: Could not seek to exercise position\n");
        return;
    }


    // Read weight and reps with error checking
    if(fread(&data->weight, sizeof(double), 1, user_context->userFILE) != 1) {
        printf("Error reading weight for exercise %d (%s)\n", exercise_index, exercises_c[exercise_index].name);
		return;
	}
    if (fread(&data->reps, sizeof(int), 1, user_context->userFILE) != 1) {
        printf("Error reading reps for exercise %d (%s)\n", exercise_index, exercises_c[exercise_index].name);
        return;
    }


    return;
}


/**@brief Reads user preferences from the user's file
 * @param userFILE Pointer to the file containing user data
 * @return user_file_header_prefs* Pointer to the user preferences structure else NULL*/
user_file_header_prefs* read_user_preferences(FILE* userFILE) {

    // Allocate memory for user preferences
    user_file_header_prefs* prefs = (user_file_header_prefs*)malloc(sizeof(user_file_header_prefs));
    if(prefs == NULL) {
        printf("Error: Could not allocate memory for user preferences\n");
        return NULL;                    	
    }


    // Seek to start of file where preferences are stored
    long position = fseek(userFILE, 0, SEEK_SET);
    if (fseek(userFILE, position, SEEK_SET) != 0) {
        printf("Error: Could not seek to exercise position\n");
        return NULL;
    }


    // Read days and time with error checking
    if (fread(prefs, sizeof(user_file_header_prefs), 1, userFILE) != 1) {
		printf("Error reading preferences\n");
		return NULL;
	}

    return prefs;
}


/**@brief Updates exercise data for a specific exercise in the user's file
 * @param userFILE Pointer to the file containing exercise data
 * @param exercise_index Index of the exercise to update
 * @param new_data New exercise data to write (weight and reps)
 * @return 0 on success, -1 on error */
int update_user_exercise_data() {
    
    const size_t RECORD_SIZE = sizeof(double) + sizeof(int);
    const size_t SKIP_PREFS = sizeof(user_file_header_prefs);
    const size_t SKIP_WORKOUT = local_userPrefs->prefered_days * sizeof(workout_days_t);

    // Validate file pointer
    if (local_userFILE == NULL) {
        printf("Error: Invalid file pointer\n");
        return -1;
    }


    for(int i = 0; i < AMOUNT_EXERCISES; i++) {

        // Calculate position for target exercise
        long position = SKIP_PREFS + SKIP_WORKOUT + i * RECORD_SIZE;

        // Seek to exercise position
        if (fseek(local_userFILE, position, SEEK_SET) != 0) {
            printf("Error: Could not seek to exercise position\n");
            return -1;
        }

        user_file_exercise_data new_data = {
			.weight = exercises_c[i].user_exercise_data->weight,
			.reps = exercises_c[i].user_exercise_data->reps,
		};

        // Write weight and reps explicitly with error checking
        if (fwrite(&new_data.weight, sizeof(double), 1, local_userFILE) != 1) {
			printf("Error writing weight for exercise %d (%s)\n",
				   i, exercises_c[i].name);
			return -1;
		}

        if (fwrite(&new_data.reps, sizeof(int), 1, local_userFILE) != 1) {
            printf("Error writing reps for exercise %d (%s)\n",
                i, exercises_c[i].name);
            return -1;
        }
    }

    fflush(local_userFILE); // Ensure data is written to disk
    return 0; // Success
}

void read_previous_user_workout_data(user_file_exercise_data* data, int index) {

    const size_t SKIP_PREFS = sizeof(user_file_header_prefs);
	const size_t RECORD_SIZE = sizeof(double) + sizeof(int);

	// Validate file pointer
	if (local_userFILE == NULL) {
		printf("Error: Invalid file pointer\n");
		return;
	}

	// Read the user preferences
	user_file_header_prefs* user_prefs = read_user_preferences(local_userFILE);

	// Calculate file position for desired exercise
	long position = SKIP_PREFS + user_prefs->prefered_days * sizeof(workout_days_t) + (1+index)*(AMOUNT_EXERCISES*RECORD_SIZE);

	// Seek to exercise position
	if (fseek(local_userFILE, position, SEEK_SET) != 0) {
		printf("Error: Could not seek to exercise position\n");
		return;
	}

    for(int i = 0; i < AMOUNT_EXERCISES; i++) {
		fread(&data[i].weight, sizeof(double), 1, local_userFILE);
		fread(&data[i].reps, sizeof(int), 1, local_userFILE);
	}
}

workout_days_t* read_user_workout_data() {
	// Read user preferences
	user_file_header_prefs* user_file_header_prefs = read_user_preferences(local_userFILE);

	const size_t SKIP_PREFS = sizeof(user_file_header_prefs);

	long position = SKIP_PREFS;

	if (local_userFILE == NULL) {
		printf("Error: Invalid file pointer\n");
	}

	// Seek to exercise position
	if (fseek(local_userFILE, position, SEEK_SET) != 0) {
		printf("Error: Could not seek to exercise position\n");
	}

	workout_days_t* workout = malloc(sizeof(workout_days_t) * user_file_header_prefs->prefered_days);

	// Read workout days with error checking
	if (fread(workout, sizeof(workout_days_t), user_file_header_prefs->prefered_days, local_userFILE) != user_file_header_prefs->prefered_days) {
		printf("Error reading workout days from file\n");
	}

	return workout;
}

void update_user_preferences(user_file_header_prefs* user_prefs) {

    // Seek to start of file where preferences are stored
    
    long position = fseek(local_userFILE, 0, SEEK_SET);

    // Write user preferences to file
	if (fwrite(&user_prefs->prefered_days, sizeof(int), 1, local_userFILE) != 1) {
		printf("\nError writing preferences to file!\n");
	}

	if (fwrite(&user_prefs->perfered_time, sizeof(int), 1, local_userFILE) != 1) {
		printf("\nError writing preferences to file!\n");
	}

	if (fwrite(&user_prefs->workout_counter, sizeof(int), 1, local_userFILE) != 1) {
		printf("\nError writing preferences to file!\n");
	}

	fflush(local_userFILE);
}

int update_user_workout_data(workout_days_t *workout) {
    // We implicitly assume that workout_days* has a length equal to the user's preferred days
    // This is a bit of a hack, but it's the easiest way to do it

    user_file_header_prefs* user_file_header_prefs = read_user_preferences(local_userFILE);

    const size_t SKIP_PREFS = sizeof(user_file_header_prefs);

    long position = SKIP_PREFS;

    if (local_userFILE == NULL) {
        printf("Error: Invalid file pointer\n");
    }

    // Seek to exercise position
    if (fseek(local_userFILE, position, SEEK_SET) != 0) {
		printf("Error: Could not seek to exercise position\n");
        return -1;
	}

    // Write workout days to file explicitly with error 
    if (fwrite(workout, sizeof(workout_days_t), user_file_header_prefs->prefered_days, local_userFILE) != user_file_header_prefs->prefered_days) {
        printf("\nError writing workout days to file!\n");
        return -1;
    }

    fflush(local_userFILE);
    return 0;
}

int write_user_data_post_workout() {
    
    // Save workout data post workout at end of file:
    // FILE - HEADER - PREFS
    // FILE - HEADER - DATA
    // FILE - EXERCISE DATA
    // FILE - PREVIOUS EXERCISE DATA

    // SEEK to end
    // WRITE to end
    // SEEK to workout_counter
    // WRITE to workout_counter workout_counter + 1

    user_file_header_prefs* userPrefs = read_user_preferences(local_userFILE);

    userPrefs->workout_counter++;

    update_user_preferences(userPrefs);

    if (fseek(local_userFILE, 0, SEEK_END)) return 1;

    for (int i = 0; i < AMOUNT_EXERCISES; i++) {
		if (fwrite(&exercises_c[i].user_exercise_data->weight, sizeof(double), 1, local_userFILE) != 1) {
			printf("Error writing weight for exercise %d (%s)\n",
				   i, exercises_c[i].name);
			return -1;
		}

		if (fwrite(&exercises_c[i].user_exercise_data->reps, sizeof(int), 1, local_userFILE) != 1) {
			printf("Error writing reps for exercise %d (%s)\n",
				   i, exercises_c[i].name);
			return -1;
		}
	}


    fflush(local_userFILE);

    free(userPrefs);

    return 0;
    /*FILE* userFILE = fopen(userprofile_path, "rb+");

    const size_t RECORD_SIZE = sizeof(double) + sizeof(int);
    const size_t SKIP_PREFS = sizeof(UserPreferences_t);

    // Calculate position for target exercise
    long position = SKIP_PREFS + AMOUNT_EXERCISES * RECORD_SIZE;

    // Validate file pointer
    if (userFILE == NULL) {
        printf("Error: Invalid file pointer\n");
        return -1;
    }

    // Seek to workout counter position
    if (fseek(userFILE, position, SEEK_SET) != 0) {
        printf("Error: Could not seek to workout counter position\n");
        return -1;
    }

    if (fread(&userfile_workout_counter, sizeof(int), 1, userFILE) != 1) {
        printf("Error reading workout counter in userfile\n");
    }

    // Det virker ikke rigtigt det her
    userfile_workout_counter++;

    if (fseek(userFILE, position, SEEK_SET) != 0) {
        printf("Error: Could not seek to workout counter position\n");
        return -1;
    }

    if (fwrite(&userfile_workout_counter, sizeof(int), 1, userFILE) != 1) {
        printf("Error updating workout counter in userfile\n");
        return -1;
    }

    for (int i = 0; i < AMOUNT_EXERCISES; i++) {

        // Seek to exercise position
        if (fseek(userFILE, 0, SEEK_END) != 0) {
            printf("Error: Could not seek to exercise position\n");
            return -1;
        }

        if (fwrite(&exercises_c[i].user_exercise_data->weight, sizeof(double), 1, userFILE) != 1) {
            printf("Error writing weight for exercise %d (%s)\n",
                   i, exercises_c[i].name);
            return -1;
        }

        if (fwrite(&exercises_c[i].user_exercise_data->reps, sizeof(int), 1, userFILE) != 1) {
            printf("Error writing reps for exercise %d (%s)\n",
                   i, exercises_c[i].name);
            return -1;
        }
    }
    fflush(userFILE);
    return 0;
    */
}
