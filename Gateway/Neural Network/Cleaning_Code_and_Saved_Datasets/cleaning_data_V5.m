%% Normalization of Data

load("dataset_timetable_new_dates_30min_partiallyFilledSkewedDist_CleanTurbiditypHOutliers.mat")

datasetcapstone.Temperature = normalize(datasetcapstone.Temperature);
datasetcapstone.Turbidity = normalize(datasetcapstone.Turbidity);
datasetcapstone.pH = normalize(datasetcapstone.pH);