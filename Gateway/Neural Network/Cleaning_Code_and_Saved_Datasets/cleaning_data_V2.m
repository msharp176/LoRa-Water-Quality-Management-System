% Converted the 10min interval dataset to 30min interval
% Removed timestamps not at the hour or 30 min mark.

load dataset_timetable_new_dates_10min.mat

%% Converting data to every 10 min

my_height = height(new_datasetcapstone);
new_height = int32(height(new_datasetcapstone)/3);

first = new_datasetcapstone.Timestamp(1);
last = new_datasetcapstone.Timestamp(end);

Timestamp = first + 60 * 30 * seconds(0:new_height);
Timestamp = Timestamp';

%%
datasetcapstone_30min = timetable(Timestamp, NaN(height(Timestamp),1), NaN(height(Timestamp),1), NaN(height(Timestamp),1), 'VariableNames', {'Temperature', 'pH', 'Turbidity'});

%%

for n = 1:height(datasetcapstone_30min)
    if datasetcapstone_30min.Timestamp(n) == new_datasetcapstone.Timestamp(3*n-2)
        datasetcapstone_30min.Temperature(n) = new_datasetcapstone.Temperature(3*n-2);
        datasetcapstone_30min.pH(n) = new_datasetcapstone.pH(3*n-2);
        datasetcapstone_30min.Turbidity(n) = new_datasetcapstone.Turbidity(3*n-2);
    end
end