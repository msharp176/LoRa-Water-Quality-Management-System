% Fills in missing timestamps.
% Before: timestamps at intervals of 10min and 30min.
% After: timestamps at intervals of 10min with missing
%          data filled with NaN 

%% Clear workspace, command window and load dataset
clc
clear

load dataset_timetable.mat

%% Make timetable of unique Timestamps

unique_datasetcapstone = unique(datasetcapstone);

%% Make new, empty timetable filling missing Timestamps

first = unique_datasetcapstone.Timestamp(1);
last = unique_datasetcapstone.Timestamp(end);
interval = minutes(10);
range = last - first;
new_n_elements = range/interval;
n_elements = height(unique_datasetcapstone.Timestamp);
Timestamp = first + 60 * 10 * seconds(0:new_n_elements);
Timestamp = Timestamp';

clear first, clear last, clear interval, clear range

new_datasetcapstone = timetable(Timestamp, NaN(new_n_elements+1,1), NaN(new_n_elements+1,1), NaN(new_n_elements+1,1), 'VariableNames', {'Temperature', 'pH', 'Turbidity'});

clear Timestamp

%% Print first and last timestamp of old and new timetable for confirmation

% fprintf('Original Start Timestamp: %s\n', datetime(datasetcapstone.Timestamp(1)));
% fprintf('New Start Timestamp: %s\n', datetime(new_datasetcapstone.Timestamp(1)));
% fprintf('Original End Timestamp: %s\n', datetime(datasetcapstone.Timestamp(end)));
% fprintf('New End Timestamp: %s\n', datetime(new_datasetcapstone.Timestamp(end)));

%% Fill ph, Temperature, and Turbidity of new timetable (part 1)

for n = 43366:new_n_elements
    for v = 26904:n_elements
        if unique_datasetcapstone.Timestamp(v) == new_datasetcapstone.Timestamp(n)
            new_datasetcapstone.Temperature(n) = unique_datasetcapstone.Temperature(v);
            new_datasetcapstone.pH(n) = unique_datasetcapstone.pH(v);
            new_datasetcapstone.Turbidity(n) = unique_datasetcapstone.Turbidity(v);
            break
        end
        if unique_datasetcapstone.Timestamp(v) > new_datasetcapstone.Timestamp(n)
            break
        end
    end
end

%% Fill ph, Temperature, and Turbidity of new timetable (part 2)

new_datasetcapstone.Temperature(end) = unique_datasetcapstone.Temperature(end),
new_datasetcapstone.pH(end) = unique_datasetcapstone.pH(end),
new_datasetcapstone.Turbidity(end) = unique_datasetcapstone.Turbidity(end)

%% Fill gap (Part 1) Find slope

before = 36931;
after = 41316;
before_gap = new_datasetcapstone(before,:);
after_gap = new_datasetcapstone(after,:);

% Calculating slope
Temperature_slope = (new_datasetcapstone(after,:).Temperature-new_datasetcapstone(before,:).Temperature)/(after-before);
pH_slope = (new_datasetcapstone(after,:).pH-new_datasetcapstone(before,:).pH)/(after-before);
Turbidity_slope = (new_datasetcapstone(after,:).Turbidity-new_datasetcapstone(before,:).Turbidity)/(after-before);

% Extract the temperature values from the specified rows
temperature_values = new_datasetcapstone(36932:41315, :).Temperature;
pH_values = new_datasetcapstone(36932:41315, :).pH;
turbidity_values = new_datasetcapstone(36932:41315, :).Turbidity;

% Calculate the standard deviation
temperature_std_dev = std(temperature_values);
pH_std_dev = std(pH_values);
turbidity_std_dev = std(turbidity_values);


%% 
% Interpolate missing values between the two gaps
for i = 36932:10:41315
    new_datasetcapstone.Temperature(i) = interp1([36931, 41316], [before_gap.Temperature, after_gap.Temperature], i, 'pchip');
    new_datasetcapstone.pH(i) = interp1([36931, 41316], [before_gap.pH, after_gap.pH], i, 'pchip');
    new_datasetcapstone.Turbidity(i) = interp1([36931, 41316], [before_gap.Turbidity, after_gap.Turbidity], i, 'pchip');
end
