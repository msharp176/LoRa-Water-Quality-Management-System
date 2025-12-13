Fs = 100; % Sample rate (Hz)
T = 1/Fs;
t = -0.5:T:0.5;

signal = 50*exp(-100*abs(t)).*sign(t)

N = length(signal); % Length of the signal
Y = fft(signal); % Compute the FFT
f = (0:N-1)*(Fs/N); % Frequency vector
magnitude = abs(Y); % Magnitude of the FFT

figure;
plot(f, magnitude);
title('Frequency Spectrum of the Signal');
xlabel('Frequency (Hz)');
ylabel('Magnitude');
xlim([0 Fs/2]); % Limit x-axis to half the sample rate (Nyquist frequency)
grid on;