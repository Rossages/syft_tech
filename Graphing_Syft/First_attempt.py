# This file will convert a .txt file into appropiate columns from STM Studio.

import xlsxwriter  # this is imported to use the Excel library.
import  sys  # this is used to get the size of the data string, or see what data type something is.


# this following class will be used to abstract the data from each string i a tidy way.
# all of the indexing will be done with in this class.

class DataAbstraction:
    def __init__(self, info_string):
        self.info_string = info_string

        self.time = info_string[0]  # TODO: not sure if i will need this as i will use this as 'name' of the Data set

        # the below are all the attributes that make up each measurement
        # Lens 1
        self.US_L1_AA = info_string[1]  # US_Lens1_ActualA
        self.US_L1_AV = info_string[2]  # US_Lens1_ActualV
        self.US_L1_SP = info_string[3]  # US_Lens1_SetpointV

        # lens 2
        self.US_L2_AA = info_string[4]  # US_Lens2_ActualA
        self.US_L2_AV = info_string[5]  # US_Lens2_ActualV
        self.US_L2_SP = info_string[6]  # US_Lens2_SetpointV

        # lens 3
        self.US_L3_AA = info_string[7]
        self.US_L3_AV = info_string[8]
        self.US_L3_SP = info_string[9]

        # lens 4
        self.US_L4_AA = info_string[10]
        self.US_L4_AV = info_string[11]
        self.US_L4_SP = info_string[12]

        # lens 5
        self.US_L5_AA = info_string[13]
        self.US_L5_AV = info_string[14]
        self.US_L5_SP = info_string[15]

        # lens 6
        self.US_L6_AA = info_string[16]
        self.US_L6_AV = info_string[17]
        self.US_L6_SP = info_string[18]

        # Prefilter lenses
        self.US_Prefilt_AA = info_string[19]
        self.US_Prefilt_AV = info_string[20]
        self.US_Prefilt_SP = info_string[21]




processed_info_titles = []
raw_values = []  # list that all of our data will be chucked into to temporarily
j = 0
i = 0


# init file as read only
lenstext = open("LensMonitorLog-38927-with-L1-M.txt", "r")


# lines 0 -> 7
heading_titles = ['Version', 'time_date', 'process', 'space', 'logdata', 'Varheader', 'lenses',
                  "Id"]  # titles for the first lines in the file.


def file_len(fname):
    with fname as f:
        # Enumerate has two parts to it. The counter and the item. Hence the
        # i and l in the for loop. I only want the count! :)
        for i, l in enumerate(f):
            pass
    return int(i)


num_lines = file_len(lenstext) #this give a int for the number of data sets in the file.
time_stamped_info = []


def read_samples(timestamp):
    # this will take the string and strip all of the \t and give actual information to be processed
    # which will have all the individual values in a list.

    temp = list(map(lambda x: x.split("\t"), timestamp))
    return temp


# We have to reopen the text file ones it has been closed.
lenstext = open("LensMonitorLog-38927-with-L1-M.txt", "r")


for line in lenstext:

    if j <= 7:
        processed_info_titles.append((heading_titles[j] + "::" + line))

        if j == 6:
            # this is the lens string.
            # below makes the lens_titles a seperate list.
            lens_titles = read_samples([line[4:]])  # 4: is to strip the D: from the list.
            print("lens_titles: ", lens_titles[0])
            print(len(lens_titles[0]))

        if j == 7:
            # this is for the funny title things.
            funny_titles = read_samples([line[3:]])  # 3: is to strip the D: from the list.
            # this guy also give us time(ms) title.
            print(funny_titles)

    if 8 <= j <= num_lines:

            # This is where all the good important data comes from.
            raw_values.append(line[3:])  # this will strip the D: off all of the data points.( and makes a copy of item)
            # below line finally supplies the
            raw_values = read_samples(raw_values)  # remove ething on raw_values list so that the next list can be processed.
            time_stamped_info.insert((j-8), raw_values) # this inserts the rawdata into the num_lines sized list. creating a list of lists

            # TODO: :( - each individual list is embedded in one extra list..


            raw_values = []  # empty list

    j += 1  # this will increment j after ever time line in lenstext is incremented


# ******* From here we will take individual parts of the data so that they can be plotted. *********

#for i in time_stamped_info:


# from here we can take away one lot of lists.
time_stamped_info = time_stamped_info[4000]  # this is for all the different lists. ie. Time(ms)
testing = time_stamped_info[0]

# below line will be replaced by the class indexing :)
blah = testing[1]  # at this stage i can actually abstract data from the list


#time = '0.28'
time = DataAbstraction(testing)

print("time_stamped", blah)
print("t :", time.US_L1_AA)
print("Num lines ", num_lines)
print("\n")
print(processed_info_titles)
print("\n")


