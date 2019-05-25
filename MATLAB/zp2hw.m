function [b a] = zp2hw(zeroes, poles);

% Zero-pole vector inputs
% w (frequencies) and H(w) (modulus) outputs

%% Requires opening a figure and use of hold on;

    G = 1; % Gain factor
    [b a] = zp2tf(zeroes', poles, G);
    [h w] = freqz(b, a);

    bsum = sum(b); % Gain factor
    asum = sum(a);
    plot(w, abs(h)/(bsum/asum));

end

