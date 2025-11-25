// .tcc file, template header file, see here: https://stackoverflow.com/a/10632266/3423623

#ifndef sampleReading_h
#define sampleReading_h

// Takes the average of the vales.
// In the case of booleans, if any of the sampeles are false, when it truncates it will round down to false
template <typename T>
void sampleReading(T &result, T reading, T *arr, int &counter, const int arrSize)
{
    arr[counter] = reading;
    counter++;
    if (counter >= arrSize)
    {
        double total = 0; // choosing double because max we read is float and integers we can also fit into doubles then convert back to integers.
        for (int i = 0; i < arrSize; i++)
        {
            total += (double)arr[i];
        }

        total = total / arrSize;
        result = (T)total;
        counter = 0;
    }
}
#endif