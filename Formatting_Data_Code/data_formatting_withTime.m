load("dataset_cleaned.mat")

%% Switch from timetable to cell for trainnet formatting

data = timetable2table(datasetcapstone);
data = table2cell(data);
date_vector = string(data(:,1));
date_vector = datevec(date_vector);
data = cell2mat(data(:,2:4));
% date_vector = num2cell(date_vector);
data = horzcat(date_vector, data);

%%
n1 = 169;
n2 = 93;
% n1 = 31
% n2 = 507

CellCellArray = cell(n2,9);
CellArray = cell(n1,1);

%%
for n = 1:n1
    initial = (n-1)*n2+1;
    final = n*n2;
    CellCellArray = data(initial:final,:);
    CellArray{n} = CellCellArray;
end

%%

data = CellArray;