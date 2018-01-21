import xlsxwriter

workbook = xlsxwriter.Workbook('hello.xlsx')
worksheet = workbook.add_worksheet()

worksheet.write('A1', 'Hello world')

workbook.close()

#row = 1
#col = 0

# these are examples for how to write to an excel spreadsheet

'''
workbook = xlsxwriter.Workbook('hello.xlsx')
worksheet = workbook.add_worksheet()

for time in time_stamped_info:
    print(time)
    worksheet.write(row, col, time)



workbook.close()
‘''
