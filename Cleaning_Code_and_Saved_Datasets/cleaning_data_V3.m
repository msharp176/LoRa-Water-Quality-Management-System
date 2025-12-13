% Fills every 10th missing data point of long gap of missing data.

%%

load("dataset_timetable_new_dates_30min.mat")

%% Calculates slope and standard deviation

% Variables for element before and after missing gap
n_before_gap = 12311;
n_after_gap = 13773;
before_gap = datasetcapstone(n_before_gap,:);
after_gap = datasetcapstone(n_after_gap,:);

% Calculates slope of missing gap
n_average = 10;
before_gap_average = mean(datasetcapstone(n_before_gap-n_average:n_before_gap,:));
after_gap_average = mean(datasetcapstone(n_after_gap+n_average:n_after_gap+n_average,:));
temperature_slope = (after_gap_average.Temperature-before_gap_average.Temperature)/(n_after_gap-n_before_gap);
turbidity_slope = (after_gap_average.Turbidity-before_gap_average.Turbidity)/(n_after_gap-n_before_gap);
pH_slope = (after_gap_average.pH-before_gap_average.pH)/(n_after_gap-n_before_gap);

% Calculates standard deviation of last 1000 elements before missing gap
temperature_sd = std(datasetcapstone.Temperature(n_before_gap-1000:n_before_gap,1),'omitnan');
turbidity_sd = std(datasetcapstone.Turbidity(n_before_gap-1000:n_before_gap,1),'omitnan');
pH_sd = std(datasetcapstone.pH(n_before_gap-1000:n_before_gap,1),'omitnan');

% % Calculates mean of the last 1000 elements before missing gap
% temperature_mean = mean(datasetcapstone.Temperature(n_before_gap-1000:n_before_gap,1),'omitnan');
% turbidity_mean = mean(datasetcapstone.Turbidity(n_before_gap-1000:n_before_gap,1),'omitnan');
% pH_mean = mean(datasetcapstone.pH(n_before_gap-1000:n_before_gap,1),'omitnan');

%% Partially fills data

% Fills every 1 for every 10 elements with points normally distributed about the calculated slope
for n = n_before_gap:10:n_after_gap
    location = n+randi([1,10]);
    
    % Generates random number 

    z_score = norminv(rand());
    datasetcapstone.Temperature(location) = before_gap_average.Temperature + temperature_slope * (location - n_before_gap) + z_score * temperature_sd;
    datasetcapstone.Turbidity(location) = before_gap_average.Turbidity + turbidity_slope * (location - n_before_gap) + z_score * turbidity_sd;
    datasetcapstone.pH(location) = before_gap_average.pH + pH_slope * (location - n_before_gap) + z_score * pH_sd;
end

%% Plots filled data

filled_data = datasetcapstone(n_before_gap:n_after_gap,:);

date = filled_data.Timestamp;
date = datetime(date,'InputFormat','MM/dd/yyyy HH:mm');
pH = filled_data.pH;
Temperature = filled_data.Temperature;
Turbidity = filled_data.Turbidity;

Y = [pH, Temperature, Turbidity];
X = date;

Ylabels = ["pH","Temperature (" + char(176) + "C)","Turbidity (NTUs)"];
stackedplot(X,Y,"Title","Filled Data","DisplayLabels",Ylabels)