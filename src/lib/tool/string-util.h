
#ifndef STRINGUTIL_H
#define STRINGUTIL_H
#include <QString>
#include "lib/tool/xmath.h"

class StringUtil {
public:
    /**
   * Calculates the similarity (a number within 0 and 1) between two strings.
   */
    static double similarity(QString first, QString second) {
        double max_length = std::max(first.length(), second.length());
        if (max_length > 0) {
            return (max_length - getEditDistance(first, second)) / max_length;
        }
        return 1.0;
    }

private:
        // Example implementation of the Levenshtein Edit Distance
        // See http://rosettacode.org/wiki/Levenshtein_distance#Java
    static int getEditDistance(QString first, QString second)
    {
            int m = first.length();
            int n = second.length();

            int T[m + 1][n + 1];
            for (int i = 1; i <= m; i++) {
                T[i][0] = i;
            }

            for (int j = 1; j <= n; j++) {
                T[0][j] = j;
            }

            for (int i = 1; i <= m; i++) {
                for (int j = 1; j <= n; j++) {
                    int weight = first[i - 1] == second[j - 1] ? 0: 1;
                    T[i][j] = std::min(std::min(T[i-1][j] + 1, T[i][j-1] + 1), T[i-1][j-1] + weight);
                }
            }

            return T[m][n];
    }
};

#endif // STRINGUTIL_H
