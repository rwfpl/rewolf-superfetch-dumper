del /S *.unp
del /S *.txt
for /R %%c in (*.db) do SuperFetch.exe %%c > %%c.txt