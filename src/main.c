#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include <stdarg.h>

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof(*x))
#define LOG_ERROR(...) (fprintf(stderr, __VA_ARGS__))

static const int integral_approximation_result_max_length = 256;

int logging_vprintf(char *format, va_list args_ptr) {
    int result = vprintf(format, args_ptr);
    if(result < 0) LOG_ERROR("Unable to write to stdout\n");
    return result;
}

int logging_printf(char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = logging_vprintf(format, args);
    va_end(args);
    return result;
}

void *logging_malloc(size_t block_size, const char *variable_name) {
    void *allocated_block = malloc(block_size);
    if (!allocated_block) LOG_ERROR("Unable to allocate memory for %s\n", variable_name);
    return allocated_block;
}

int read_double(double *result, const char *variable_name) {
    if (logging_printf("Enter %s:\n", variable_name) < 0) return -1;
    if (scanf("%lf", result) != 1) {
        LOG_ERROR("Unable to read %s\n", variable_name);
        return -1;
    }
    return 0;
}

void free_string_array(char **arr, int arr_length) {
    for (int i = 0; i < arr_length; i++)
        free(arr[i]);
    free(arr);
}

int print_string_array(char **arr, int arr_length) {
    for (int i = 0; i < arr_length; i++)
        if (logging_printf("%s\n", arr[i]) < 0)
            return -1;
    return 0;
}

struct interval_t {
    double left_bound;
    double right_bound;
};

double get_interval_midpoint(const struct interval_t *interval) {
    return (interval->left_bound + interval->right_bound) / 2;
}

double get_interval_length(const struct interval_t *interval) {
    return interval->right_bound - interval->left_bound;
}

int read_interval(struct interval_t *interval) {
    if (read_double(&(interval->left_bound), "interval's left bound") != 0)
        return -1;
    if (interval->left_bound < 0) {
        LOG_ERROR("Interval's left bound must be greater than or equal to 0\n");
        return -1;
    }
    if (read_double(&(interval->right_bound), "interval's right bound") != 0)
        return -1;
    if (interval->left_bound > M_PI) {
        LOG_ERROR("Interval's right bound must be less than or equal to pi\n");
        return -1;
    }
    if (interval->left_bound >= interval->right_bound) {
        LOG_ERROR("Interval's right bound must be greater than the left one\n");
        return -1;
    }
    return 0;
}

double f(double x) {
    return sin(x);
}

double get_midpoint_rectangle_area(const struct interval_t *interval) {
    return get_interval_length(interval) * f(get_interval_midpoint(interval));
}

double get_area_using_simpsons_rule(const struct interval_t *interval) {
    return get_interval_length(interval) / 6.0 *
           (f(interval->left_bound) + 4.0 * f(get_interval_midpoint(interval)) + f(interval->right_bound));
}

double approximate_integral_by_partitioning(
        const struct interval_t *interval,
        unsigned int partition_count,
        double (*partition_area_approximator)(const struct interval_t *)
) {
    double partition_length = get_interval_length(interval) / partition_count;
    double result = 0;
    struct interval_t partition = {interval->left_bound, interval->left_bound + partition_length};
    for (int i = 0; i < partition_count; i++) {
        result += partition_area_approximator(&partition);
        partition.left_bound += partition_length;
        partition.right_bound += partition_length;
    }
    return result;
}

char *create_integral_approximation_result(const struct interval_t *interval, unsigned int partition_count) {
    char *result = logging_malloc(
            integral_approximation_result_max_length * sizeof(char),
            "integral approximation result"
    );
    if (!result) return NULL;
    if (!snprintf(
            result,
            integral_approximation_result_max_length,
            "%d %.5f %.5f",
            partition_count,
            approximate_integral_by_partitioning(interval, partition_count, get_midpoint_rectangle_area),
            approximate_integral_by_partitioning(interval, partition_count, get_area_using_simpsons_rule)
    )) {
        LOG_ERROR("Unable to write result to string\n");
        free(result);
        return NULL;
    }
    return result;
}

char **create_integral_approximation_result_array(
        const struct interval_t *interval,
        unsigned int *partition_count_arr,
        int arr_length
) {
    char **results = logging_malloc(arr_length * sizeof(char *), "result array");
    if (!results) return NULL;
    for (int i = 0; i < arr_length; i++) {
        results[i] = create_integral_approximation_result(interval, partition_count_arr[i]);
        if (!results[i]) {
            LOG_ERROR("Error occurred in experiment #%d\n", i);
            free_string_array(results, i);
            return NULL;
        }
    }
    return results;
}

int main() {
    unsigned int partition_count_arr[] = {6, 10, 20, 100, 500, 1000};
    int experiments_count = ARRAY_LENGTH(partition_count_arr);
    struct interval_t interval = {0.0, 0.0};
    if (read_interval(&interval) != 0) return 1;
    char **results = create_integral_approximation_result_array(
            &interval,
            partition_count_arr,
            experiments_count
    );
    if (!results) return 1;
    if (print_string_array(results, experiments_count) != 0) {
        free_string_array(results, experiments_count);
        return 1;
    }
    free_string_array(results, experiments_count);
    return 0;
}
