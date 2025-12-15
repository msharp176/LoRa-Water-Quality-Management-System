load("dataset_cleaned_notNormalized_smooth.mat")

%% Switch from timetable to cell for trainnet formatting

data = timetable2table(datasetcapstone);
data = table2cell(data);
% date_vector = string(data(:,1));
% date_vector = datevec(date_vector);
data = cell2mat(data(:,2:4));
% date_vector = num2cell(date_vector);
% data = horzcat(date_vector; data);

%%
% 3 x 13^2 x 31
divisions = 13^2;
parts = size(datasetcapstone,1)/divisions;

CellCellArray = cell(507,3);
CellArray = cell(divisions,1);

%%
for n = 1:divisions
    initial = (n-1)*parts+1;
    final = n*parts;
    CellCellArray = data(initial:final,:);
    CellArray{n} = CellCellArray;
end

%%

data = CellArray;