'create the excel object
	Set objExcel = CreateObject("Excel.Application") 

'view the excel program and file, set to false to hide the whole process
	objExcel.Visible = True 

'add a new workbook
	Set objWorkbook = objExcel.Workbooks.Add 

'set a cell value at row 3 column 5
	objExcel.Cells(3,5).Value = "new value"