% Fills 1 random element (1-10) for every 10 elements of long gap of missing data
% using skewness and kurtosis of last 1000 elements before gap

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

% Calculates standard deviation of last 1000 elements before big gap
temperature_sd = std(datasetcapstone.Temperature(n_before_gap-1000:n_before_gap,1),'omitnan');
turbidity_sd = std(datasetcapstone.Turbidity(n_before_gap-1000:n_before_gap,1),'omitnan');
pH_sd = std(datasetcapstone.pH(n_before_gap-1000:n_before_gap,1),'omitnan');

%%
% Calculates kurtosis of last 1000 elements before big gap
temperature_kurtosis = kurtosis(datasetcapstone.Temperature(n_before_gap-1000:n_before_gap,1),1);
turbidity_kurtosis = kurtosis(datasetcapstone.Turbidity(n_before_gap-1000:n_before_gap,1),1);
pH_kurtosis = kurtosis(datasetcapstone.pH(n_before_gap-1000:n_before_gap,1),1);

% Calculates skewness of last 1000 elements before big gap
temperature_skewness = skewness(datasetcapstone.Temperature(n_before_gap-1000:n_before_gap,1),1);
turbidity_skewness = skewness(datasetcapstone.Turbidity(n_before_gap-1000:n_before_gap,1),1);
pH_skewness = skewness(datasetcapstone.pH(n_before_gap-1000:n_before_gap,1),1);

% % Calculates mean of the last 1000 elements before missing gap
% temperature_mean = mean(datasetcapstone.Temperature(n_before_gap-1000:n_before_gap,1),'omitnan');
% turbidity_mean = mean(datasetcapstone.Turbidity(n_before_gap-1000:n_before_gap,1),'omitnan');
% pH_mean = mean(datasetcapstone.pH(n_before_gap-1000:n_before_gap,1),'omitnan');

%% Partially fills data

% Fills every 1 for every 10 elements with points normally distributed about the calculated slope
for n = n_before_gap:10:n_after_gap

    location = n+randi([1,10]);
    
    for v = 1:3

        % Creates variable to dynamically change variable name
        switch v
            case 1
                variable = 'pH';
                Variable = 'pH';
            case 2
                variable = 'temperature';
                Variable = 'Temperature';
            case 3
                variable = 'turbidity';
                Variable = 'Turbidity';
        end
        
        % Generates random number between 0-1 with skewness and kurtosis
        % then gets z-score of number for generating random number
        mu = 0.5*ones(1000,1);
        sigma = 1;
        kurtosis = eval(append(variable, '_kurtosis;'));
        skew = eval(append(variable, '_skewness;'));
        r = pearsrnd(mu,sigma,skew,kurtosis);
        r = normalize(r,'norm');
        r_min = min(r);
        r = r - r_min;
        r_max = max(r);
        r = r / r_max;
        randomIndex = randi(length(r)); % Generate a random index
        randomElement = 1-r(randomIndex); % Access the random element

        z_score = norminv(randomElement);

        eval(append('datasetcapstone.(Variable)(location) = before_gap_average.(Variable) + ',variable,'_slope * (location - n_before_gap) + z_score * ',variable,'_sd;'))
        
    end
    
    %datasetcapstone.Temperature(location) = before_gap_average.Temperature + temperature_slope * (location - n_before_gap) + z_score * temperature_sd;
    %datasetcapstone.Turbidity(location) = before_gap_average.Turbidity + turbidity_slope * (location - n_before_gap) + z_score * turbidity_sd;
    %datasetcapstone.pH(location) = before_gap_average.pH + pH_slope * (location - n_before_gap) + z_score * pH_sd;
end