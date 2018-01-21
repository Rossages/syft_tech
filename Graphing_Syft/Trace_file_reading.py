
import xlsxwriter  # this is imported to use the Excel library.

# init for the excel worksheet
workbook = xlsxwriter.Workbook('trace_hello.xlsx')
worksheet = workbook.add_worksheet()


# init file as read only
tracetext = open("SIM negative wet-38927-20171114-155926.trace", "r")
proc_info = []

for line in tracetext:
    proc_info.append(line.split(","))

#  these are the titles from the trace file.
trace_titles = proc_info[0]
row1 = 0

for index, item in enumerate(trace_titles):
    tracetemp = trace_titles[index]
    worksheet.write(row1, index, tracetemp)

proc_temp = []
print("hello")

tracetext.seek(0)

for index1, itemlist in enumerate(proc_info):

    time_pos = proc_info[index1]

    if index1 > 0:
        for updown, item2 in enumerate(time_pos):
            # below will write column per column
            # print(updown, item2)
            worksheet.write(index1+1, updown,  item2)
        #print(time_pos)

workbook.close()
print("Wooh")



